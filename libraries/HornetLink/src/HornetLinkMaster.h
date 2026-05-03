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
#include "HornetLinkBase.h"
#include "HornetLinkMode.h"
#include "HornetLinkImport.h"

// Maximum number of slave addresses the master will probe.
static constexpr uint8_t kHL_MaxSlaves = 16;

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
        // Forward mode frame to all known slaves on the bus.
        for (uint8_t i = 0; i < slaveCount_; i++) {
            sendBusMode(slaveAddrs_[i], static_cast<uint8_t>(newMode));
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
        for (uint8_t i = 0; i < slaveCount_; i++) {
            sendBusData(slaveAddrs_[i], fwdBuf_, static_cast<uint8_t>(fwdLen_));
        }
        fwdLen_ = 0;
    }

    // ── RS-485 sub-bus ──────────────────────────────────
    void pollBus() {
        // Periodically probe for new slaves (every ~100 ms).
        uint32_t now = millis();
        if (now - lastPollMs_ < 100) return;
        lastPollMs_ = now;

        // Probe next candidate address (round-robin 1-16).
        uint8_t addr = probeAddr_++;
        if (probeAddr_ > kHL_MaxSlaves) probeAddr_ = 1;

        // Skip already-known slaves (already polled via data forwarding).
        sendBusProbe(addr);
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
        uint16_t crc = hl_crc16(header + 1, 4); // crc over dst,src,len
        crc = /* extend over payload */ [&]() -> uint16_t {
            uint16_t c = crc;
            for (uint8_t i = 0; i < payloadLen; i++) {
                c ^= static_cast<uint16_t>(payload[i]) << 8;
                for (uint8_t b = 0; b < 8; b++)
                    c = (c & 0x8000) ? ((c << 1) ^ 0x1021) : (c << 1);
            }
            return c;
        }();

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
    uint8_t  slaveAddrs_[kHL_MaxSlaves] = {};
    uint8_t  slaveCount_                = 0;
    uint8_t  probeAddr_                 = 1;
    uint32_t lastPollMs_                = 0;
};
