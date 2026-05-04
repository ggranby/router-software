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

// How long (ms) after the last probe-ack before a known-alive slave is
// considered offline (must be > kHL_KeepaliveMs * miss count tolerated).
static constexpr uint32_t kHL_OfflineTimeoutMs = 3000;

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
        readBus();
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

    // ── Frame-boundary parser state ──────────────────────────────────────────
    // DCS-BIOS export stream format:
    //   Sync header : 0x55 0x55 0x55 0x55
    //   Write record: ADDR_LO ADDR_HI CNT_LO CNT_HI DATA[CNT]
    //   (one or more write records per frame, followed by the next sync)
    //
    // We detect the start of each new sync header and flush the accumulated
    // prior frame to all alive RS-485 slaves at that point.
    enum class FwdState : uint8_t { SYNC_WAIT, ADDR_LO, ADDR_HI, CNT_LO, CNT_HI, DATA };

    uint8_t  fwdBuf_[512];                  // one full DCS-BIOS export frame
    uint16_t fwdLen_      = 0;
    FwdState fwdState_    = FwdState::SYNC_WAIT;
    uint8_t  syncCount_   = 0;              // consecutive 0x55 bytes seen
    uint16_t fwdDataLeft_ = 0;              // bytes remaining in current write-record

    void appendFwd(uint8_t b) {
        if (fwdLen_ < static_cast<uint16_t>(sizeof(fwdBuf_)))
            fwdBuf_[fwdLen_++] = b;
    }

    void forwardToBus(uint8_t b) {
        // Count consecutive 0x55 bytes to detect the 4-byte DCS-BIOS sync header.
        syncCount_ = (b == 0x55) ? (syncCount_ + 1) : 0;

        if (syncCount_ == 4) {
            // New frame start detected.  The 3 sync bytes before this one were
            // already appended in prior calls — strip them from the prior frame.
            if (fwdLen_ >= 3) fwdLen_ -= 3;
            if (fwdLen_ > 0) flushToSlaves();
            fwdLen_ = 0;

            // Prime buffer with the full 4-byte sync header.
            fwdBuf_[0] = fwdBuf_[1] = fwdBuf_[2] = fwdBuf_[3] = 0x55;
            fwdLen_      = 4;
            fwdState_    = FwdState::ADDR_LO;
            fwdDataLeft_ = 0;
            syncCount_   = 0;
            return;
        }

        appendFwd(b);

        switch (fwdState_) {
        case FwdState::SYNC_WAIT:
            // Still searching for the first sync — nothing else to do.
            break;
        case FwdState::ADDR_LO:
            fwdState_ = FwdState::ADDR_HI;
            break;
        case FwdState::ADDR_HI:
            fwdState_ = FwdState::CNT_LO;
            break;
        case FwdState::CNT_LO:
            fwdDataLeft_  = b;
            fwdState_     = FwdState::CNT_HI;
            break;
        case FwdState::CNT_HI:
            fwdDataLeft_ |= static_cast<uint16_t>(b) << 8;
            fwdState_     = (fwdDataLeft_ == 0) ? FwdState::ADDR_LO : FwdState::DATA;
            break;
        case FwdState::DATA:
            if (--fwdDataLeft_ == 0)
                fwdState_ = FwdState::ADDR_LO;
            break;
        }
    }

    void flushToSlaves() {
        if (fwdLen_ == 0) return;
        // Iterate alive bitmap and forward the complete frame to every discovered slave.
        for (uint16_t addr = 1; addr <= 254; addr++) {
            if (isAlive(static_cast<uint8_t>(addr))) {
                // sendBusData prepends one type byte before writeBusFrame (uint8_t len),
                // so cap each chunk at 254 data bytes to avoid uint8_t overflow.
                uint16_t remaining = fwdLen_;
                uint16_t offset    = 0;
                while (remaining > 0) {
                    uint8_t chunk = static_cast<uint8_t>(
                        remaining > 254 ? 254 : remaining);
                    sendBusData(static_cast<uint8_t>(addr),
                                fwdBuf_ + offset, chunk);
                    offset    += chunk;
                    remaining -= chunk;
                }
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
            // Offline detection: drop slave if ack has not been seen recently.
            if (now - lastAckMs_[addr - 1] >= kHL_OfflineTimeoutMs) {
                unregisterSlave(addr);
                return;  // skip keepalive — will be re-probed next kHL_ScanRetryMs
            }
            // Periodically keep-alive poll known slaves.
            if (now - lastProbeMs_[addr - 1] >= kHL_KeepaliveMs) {
                lastProbeMs_[addr - 1] = now;
                sendBusProbe(addr);
            }
        }
    }

    // ── RS-485 bus reader ──────────────────────────────────────────────────────
    // Minimal frame parser for slave → master responses.
    // Frame: [STX=0xFE][dst][src][len_lo][len_hi][payload...][crc_lo][crc_hi]
    // We only need to handle:
    //   kRS485_PROBE_ACK (0x11) — slave responded to a probe
    //   kRS485_IMPORT    (0x30) — slave relaying an import line to the PC
    //   kRS485_MODE_ACK  (0x41) — slave acknowledged a mode change
    enum class BusRxState : uint8_t {
        IDLE, DST, SRC, LEN_LO, LEN_HI, PAYLOAD, CRC_LO, CRC_HI
    };
    BusRxState busRxState_  = BusRxState::IDLE;
    uint8_t    busRxDst_    = 0;
    uint8_t    busRxSrc_    = 0;
    uint16_t   busRxLen_    = 0;
    uint16_t   busRxLeft_   = 0;
    uint8_t    busRxBuf_[64];   // large enough for probe-ack + short imports
    uint8_t    busRxBufPos_ = 0;
    uint16_t   busRxCrcRun_ = 0;
    uint8_t    busRxCrcLo_  = 0;

    void readBus() {
        while (bus_.available()) {
            uint8_t b = static_cast<uint8_t>(bus_.read());
            switch (busRxState_) {
            case BusRxState::IDLE:
                if (b == kRS485_STX) busRxState_ = BusRxState::DST;
                break;
            case BusRxState::DST:
                busRxDst_    = b;
                busRxCrcRun_ = hl_crc16(&b, 1);  // CRC starts at dst
                busRxState_  = BusRxState::SRC;
                break;
            case BusRxState::SRC:
                busRxSrc_    = b;
                busRxCrcRun_ = crc16Extend(busRxCrcRun_, &b, 1);
                busRxState_  = BusRxState::LEN_LO;
                break;
            case BusRxState::LEN_LO:
                busRxLen_    = b;
                busRxCrcRun_ = crc16Extend(busRxCrcRun_, &b, 1);
                busRxState_  = BusRxState::LEN_HI;
                break;
            case BusRxState::LEN_HI:
                busRxLen_   |= static_cast<uint16_t>(b) << 8;
                busRxCrcRun_ = crc16Extend(busRxCrcRun_, &b, 1);
                busRxBufPos_ = 0;
                busRxLeft_   = busRxLen_;
                busRxState_  = (busRxLeft_ == 0) ? BusRxState::CRC_LO : BusRxState::PAYLOAD;
                break;
            case BusRxState::PAYLOAD:
                if (busRxBufPos_ < sizeof(busRxBuf_))
                    busRxBuf_[busRxBufPos_++] = b;
                busRxCrcRun_ = crc16Extend(busRxCrcRun_, &b, 1);
                if (--busRxLeft_ == 0)
                    busRxState_ = BusRxState::CRC_LO;
                break;
            case BusRxState::CRC_LO:
                busRxCrcLo_ = b;
                busRxState_ = BusRxState::CRC_HI;
                break;
            case BusRxState::CRC_HI: {
                uint16_t rxCrc = static_cast<uint16_t>(busRxCrcLo_) |
                                 (static_cast<uint16_t>(b) << 8);
                if (rxCrc == busRxCrcRun_ && busRxBufPos_ > 0)
                    dispatchBusFrame(busRxSrc_, busRxBuf_, busRxBufPos_);
                busRxState_ = BusRxState::IDLE;
                break;
            }
            }
        }
    }

    void dispatchBusFrame(uint8_t src, const uint8_t* payload, uint8_t len) {
        if (src == 0 || src > 254 || len == 0) return;
        uint8_t msgType = payload[0];
        switch (msgType) {
        case kRS485_PROBE_ACK:
            // Slave responded to a probe — mark alive and record ack timestamp.
            registerSlave(src);
            lastAckMs_[src - 1] = millis();
            break;
        case kRS485_MODE_ACK:
            // Slave acknowledged mode change — refresh ack timestamp.
            lastAckMs_[src - 1] = millis();
            break;
        case kRS485_IMPORT:
            // Relay the import line payload verbatim to the PC.
            if (pc_ && len > 1)
                pc_->write(payload + 1, len - 1);
            lastAckMs_[src - 1] = millis();
            break;
        default:
            break;
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
    // Per-address timestamp of last ack received (for offline-timeout).
    uint32_t lastAckMs_[254]        = {};
    uint8_t  scanAddr_              = 1;     // cursor for full-sweep scan
    uint32_t lastPollMs_            = 0;

    // Legacy compat: keep slaveCount_ for forwardToBus() callers.
    // Derived on demand from aliveMap_.
    uint8_t slaveCount() const { return aliveCount(); }
};
