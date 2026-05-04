/**
 * @file HornetLinkMaster.h
 * @brief RS-485 bus master implementation for Hornet Link.
 *
 * An RS-485 master:
 *  1. Presents itself to the PC as a normal device (handshake / pong).
 *  2. Polls slave addresses on the sub-bus using the RS-485 probe protocol.
 *  3. Forwards incoming DCS-BIOS delta frames to each addressed slave.
 *  4. Relays import lines from slaves back to the PC.
 *
 * USAGE
 * ─────────────────────────────────────────────────────────────────────
 *  HornetLinkMaster hl(Serial1, DIR_PIN);
 *
 *  void setup() {
 *      Serial.begin(500000);
 *      Serial1.begin(250000, SERIAL_8N1);
 *      hl.begin(&Serial, "MyMaster", subs, ARRAYSIZE(subs),
 *               HL_FLAG_RS485_MASTER | HL_FLAG_BIDIR);
 *  }
 *
 *  void loop() { hl.update(); }
 */

#pragma once
#include <new>
#include "HornetLinkBase.h"
#include "HornetLinkMode.h"
#include "HornetLinkImport.h"

// Maximum number of slave addresses the master will probe.
static constexpr uint8_t kHL_MaxSlaves = 254;  // full RS-485 range 0x01–0xFE

// How long to wait (ms) before retrying an address that did not respond.
static constexpr uint32_t kHL_ScanRetryMs = 5000;

// How often (ms) to check known-alive slaves for keep-alive purposes.
static constexpr uint32_t kHL_KeepaliveMs = 500;

class HornetLinkMaster : public HornetLinkModeHandler {
public:
    /**
     * @param busSerial   Serial port connected to the RS-485 bus (e.g. Serial1).
     * @param dirPin      GPIO pin that controls MAX485 /RE + DE.
     */
    HornetLinkMaster(HardwareSerial& busSerial, uint8_t dirPin)
        : bus_(busSerial), dirPin_(dirPin), importer_(/* set in begin */(Stream&)Serial) {}

    /**
     * @brief Initialise and begin the handshake with the PC.
     *
     * @param pcStream   Serial stream connected to the PC (usually &Serial).
     * @param name       Device name reported in the pong frame.
     * @param subs       Subscription array.
     * @param subCount   Number of subscriptions.
     * @param flags      Capability flags (must include HL_FLAG_RS485_MASTER).
     */
    void begin(Stream* pcStream,
               const char* name,
               const hl_subscription_t* subs,
               uint16_t subCount,
               uint8_t flags)
    {
        pc_        = pcStream;
        name_      = name;
        subs_      = subs;
        subCount_  = subCount;
        flags_     = flags;
        new (&importer_) HornetLinkImport(*pc_);
        // Send pong immediately; the PC initiates with a ping.
        // In practice, we wait for the ping in update().
    }

    /** Call from Arduino loop(). */
    void update() {
        if (!pc_) return;
        readPc();
        pollBus();
    }

    /** Send an import command to DCS via the PC serial port. */
    void sendImport(const char* control, uint16_t value) {
        importer_.sendCommand(control, value);
    }

protected:
    void onModeChange(HornetLinkMode newMode) override {
        // Forward mode frame to all currently-alive slaves on the bus.
        for (uint16_t addr = 1; addr <= 254; addr++) {
            if (isAlive(static_cast<uint8_t>(addr))) {
                sendBusMode(static_cast<uint8_t>(addr),
                            static_cast<uint8_t>(newMode));
            }
        }
    }

private:
    // ── Parse states for the PC serial stream ──────────
    enum class PcState { H0, H1, H2, FT, MODE_VAL,
                         // Pong parse not needed (master builds it, not parses it)
                         // Data frames: consume until next sync
                         DATA };

    void readPc() {
        while (pc_->available()) {
            uint8_t b = static_cast<uint8_t>(pc_->read());

            // Mode frame parser (delegated to base class)
            if (processModeByte(b, *pc_)) continue;

            // Ping detection: AA DE AD 01 → reply with pong
            if (detectPing(b)) {
                hl_sendPong(*pc_, flags_, name_, subs_, subCount_);
            }
            // Note: DCS-BIOS delta frames (0x55 sync) are forwarded to the bus.
            forwardToBus(b);
        }
    }

    // Minimal ping detector state machine
    uint8_t pingState_ = 0;
    bool detectPing(uint8_t b) {
        switch (pingState_) {
        case 0: pingState_ = (b == kHL_HDR0) ? 1 : 0; break;
        case 1: pingState_ = (b == kHL_HDR1) ? 2 : 0; break;
        case 2: pingState_ = (b == kHL_HDR2) ? 3 : 0; break;
        case 3: pingState_ = 0; return (b == kHL_PING);
        }
        return false;
    }

    // Buffer incoming PC bytes and forward to slaves when a sync is detected.
    uint8_t  fwdBuf_[256];
    uint16_t fwdLen_  = 0;
    uint8_t  syncBuf_[4] = {0x55, 0x55, 0x55, 0x55};
    uint8_t  syncMatch_  = 0;

    void forwardToBus(uint8_t b) {
        // Accumulate bytes; flush to all slaves when we see a new sync header.
        fwdBuf_[fwdLen_ % sizeof(fwdBuf_)] = b;
        fwdLen_++;
        // Simple heuristic: flush on each 0x55 run
        // A more robust implementation would parse the full write-record format.
        if (b == 0x55 && fwdLen_ > 4) {
            flushToSlaves();
        }
    }

    void flushToSlaves() {
        if (fwdLen_ == 0) return;
        // Iterate alive bitmap and forward to every discovered slave.
        for (uint16_t addr = 1; addr <= 254; addr++) {
            if (isAlive(static_cast<uint8_t>(addr))) {
                sendBusData(static_cast<uint8_t>(addr), fwdBuf_,
                            static_cast<uint8_t>(fwdLen_));
            }
        }
        fwdLen_ = 0;
    }

    // ── RS-485 sub-bus ──────────────────────────────────

    /**
     * @brief Scan all 254 possible slave addresses and poll known-alive slaves.
     *
     * Strategy:
     *  - All 254 addresses are probed on startup, one per call, in order 1–254.
     *  - Addresses that respond are marked alive (aliveMap_ bit set).
     *  - After the full initial sweep, dead addresses are re-probed once every
     *    kHL_ScanRetryMs milliseconds so newly-connected slaves are discovered.
     *  - Known-alive slaves are sent a keep-alive poll every kHL_KeepaliveMs ms.
     */
    void pollBus() {
        uint32_t now = millis();
        if (now - lastPollMs_ < 20) return;   // throttle: max ~50 probes/s
        lastPollMs_ = now;

        // Advance scan cursor across all 254 addresses.
        uint8_t addr = scanAddr_++;
        if (scanAddr_ > 254) { scanAddr_ = 1; }

        bool alive = isAlive(addr);

        if (!alive) {
            // Only re-probe dead addresses after the retry interval has elapsed.
            uint32_t lastTried = lastProbeMs_[addr - 1];
            if (now - lastTried >= kHL_ScanRetryMs) {
                lastProbeMs_[addr - 1] = now;
                sendBusProbe(addr);
                // Response will be handled in readBus() → registerSlave().
            }
        } else {
            // Periodically keep-alive poll known slaves.
            if (now - lastProbeMs_[addr - 1] >= kHL_KeepaliveMs) {
                lastProbeMs_[addr - 1] = now;
                sendBusProbe(addr);
            }
        }
    }

    /**
     * @brief Record a slave as discovered/alive on the bus.
     * Called when a pong is received from a slave during scanning.
     */
    void registerSlave(uint8_t addr) {
        if (addr == 0 || addr > 254) return;
        aliveMap_[addr / 8] |= (1u << (addr % 8));
    }

    /**
     * @brief Mark a slave as offline (no response to keep-alive).
     */
    void unregisterSlave(uint8_t addr) {
        if (addr == 0 || addr > 254) return;
        aliveMap_[addr / 8] &= ~(1u << (addr % 8));
    }

    /** @return True if the slave at @p addr is currently marked alive. */
    bool isAlive(uint8_t addr) const {
        if (addr == 0 || addr > 254) return false;
        return (aliveMap_[addr / 8] & (1u << (addr % 8))) != 0;
    }

    /** @return The number of currently discovered (alive) slaves. */
    uint8_t aliveCount() const {
        uint8_t count = 0;
        for (uint8_t i = 0; i < sizeof(aliveMap_); i++) {
            uint8_t b = aliveMap_[i];
            while (b) { count += (b & 1); b >>= 1; }
        }
        return count;
    }

    void sendBusProbe(uint8_t addr) {
        uint8_t payload[1] = { kRS485_PROBE };
        writeBusFrame(addr, payload, 1);
    }

    void sendBusData(uint8_t addr, const uint8_t* data, uint8_t len) {
        // Prepend message type byte
        uint8_t buf[257];
        buf[0] = kRS485_DATA;
        memcpy(buf + 1, data, len);
        writeBusFrame(addr, buf, len + 1);
    }

    void sendBusMode(uint8_t addr, uint8_t mode) {
        uint8_t payload[2] = { kRS485_MODE, mode };
        writeBusFrame(addr, payload, 2);
    }

    /** @brief Extend a running CRC-16/CCITT-FALSE over a byte buffer. */
    static uint16_t crc16Extend(uint16_t crc, const uint8_t* data, uint8_t len) {
        for (uint8_t i = 0; i < len; i++) {
            crc ^= static_cast<uint16_t>(data[i]) << 8;
            for (uint8_t b = 0; b < 8; b++)
                crc = (crc & 0x8000u) ? ((crc << 1) ^ 0x1021u) : (crc << 1);
        }
        return crc;
    }

    /**
     * @brief Write one RS-485 sub-bus frame.
     * Frame: [STX][dst][src=0][len_lo][len_hi][payload...][crc_lo][crc_hi]
     */
    void writeBusFrame(uint8_t dst, const uint8_t* payload, uint8_t payloadLen) {
        uint8_t header[5] = {
            kRS485_STX, dst, 0x00,
            static_cast<uint8_t>(payloadLen & 0xFF),
            static_cast<uint8_t>((payloadLen >> 8) & 0xFF)
        };
        uint16_t crc = hl_crc16(header + 1, 4);       // crc over dst,src,len
        crc = crc16Extend(crc, payload, payloadLen);   // extend over payload

        // Switch to TX
        digitalWrite(dirPin_, HIGH);
        delayMicroseconds(10);
        bus_.write(header, 5);
        bus_.write(payload, payloadLen);
        bus_.write(static_cast<uint8_t>(crc & 0xFF));
        bus_.write(static_cast<uint8_t>((crc >> 8) & 0xFF));
        bus_.flush();
        // Switch back to RX
        delayMicroseconds(10);
        digitalWrite(dirPin_, LOW);
    }

    HardwareSerial&          bus_;
    uint8_t                  dirPin_;
    Stream*                  pc_         = nullptr;
    const char*              name_       = "";
    const hl_subscription_t* subs_       = nullptr;
    uint16_t                 subCount_   = 0;
    uint8_t                  flags_      = 0;
    HornetLinkImport         importer_;

    // ── Bus scan state ──────────────────────────────────
    // Bit-map of alive slave addresses (bit N set = address N is alive).
    // 256 bits = 32 bytes covers addresses 0x00–0xFF.
    uint8_t  aliveMap_[32]          = {};
    // Per-address timestamp of last probe sent (for retry / keep-alive).
    uint32_t lastProbeMs_[254]      = {};
    uint8_t  scanAddr_              = 1;     // cursor for full-sweep scan
    uint32_t lastPollMs_            = 0;

    // Legacy compat: keep slaveCount_ for forwardToBus() callers.
    // Derived on demand from aliveMap_.
    uint8_t slaveCount() const { return aliveCount(); }
};
