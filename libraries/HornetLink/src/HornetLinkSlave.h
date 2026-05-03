/**
 * @file HornetLinkSlave.h
 * @brief RS-485 bus slave implementation for Hornet Link.
 *
 * A slave device:
 *  1. Listens on the RS-485 bus for frames addressed to its bus address.
 *  2. Applies subscriptions to incoming DCS-BIOS delta frames.
 *  3. Calls the user-supplied onStateUpdate() callback when a subscribed
 *     address changes.
 *  4. Can send import lines back to the master (→ PC → DCS).
 *
 * USAGE
 * ─────────────────────────────────────────────────────────────────────
 *  HornetLinkSlave hl(Serial1, DIR_PIN);
 *
 *  void onUpdate(uint16_t addr, uint16_t val) { ... }
 *
 *  void setup() {
 *      Serial1.begin(250000, SERIAL_8N1);
 *      hl.begin(BUS_ADDR, "PanelName", subs, ARRAYSIZE(subs),
 *               HL_FLAG_RS485_SLAVE | HL_FLAG_BIDIR, onUpdate);
 *  }
 *
 *  void loop() { hl.update(); }
 */

#pragma once
#include "HornetLinkBase.h"
#include "HornetLinkMode.h"
#include "HornetLinkImport.h"

typedef void (*hl_state_cb_t)(uint16_t address, uint16_t value);

class HornetLinkSlave : public HornetLinkModeHandler {
public:
    HornetLinkSlave(HardwareSerial& busSerial, uint8_t dirPin)
        : bus_(busSerial), dirPin_(dirPin), importer_(busSerial) {}

    /**
     * @brief Initialise the slave.
     *
     * @param busAddr   This device's RS-485 bus address (1–254).
     * @param name      Device name (used in master's pong to PC).
     * @param subs      Subscription array.
     * @param subCount  Number of subscriptions.
     * @param flags     Capability flags (must include HL_FLAG_RS485_SLAVE).
     * @param cb        Callback fired when a subscribed address changes.
     */
    void begin(uint8_t busAddr,
               const char* name,
               const hl_subscription_t* subs,
               uint16_t subCount,
               uint8_t flags,
               hl_state_cb_t cb)
    {
        busAddr_   = busAddr;
        name_      = name;
        subs_      = subs;
        subCount_  = subCount;
        flags_     = flags;
        callback_  = cb;
        pinMode(dirPin_, OUTPUT);
        digitalWrite(dirPin_, LOW);
    }

    /** Call from Arduino loop(). */
    void update() {
        while (bus_.available()) {
            uint8_t b = static_cast<uint8_t>(bus_.read());
            parseFrame(b);
        }
    }

    /** Send an import command up to the master. */
    void sendImport(const char* line) {
        // Wrap in RS-485 frame with message type kRS485_IMPORT
        uint8_t buf[258];
        size_t  lineLen = strlen(line);
        buf[0] = kRS485_IMPORT;
        size_t  copyLen = lineLen < 255 ? lineLen : 255;
        memcpy(buf + 1, line, copyLen);
        buf[1 + copyLen] = '\n';
        sendBusFrame(0, buf, static_cast<uint8_t>(copyLen + 2));
    }

protected:
    void onModeChange(HornetLinkMode newMode) override {
        // Subclasses can override this to handle mode changes.
    }

private:
    // ── Sub-bus frame parse state machine ──────────────
    enum class FState {
        STX, DST, SRC, LEN_LO, LEN_HI,
        PAYLOAD, CRC_LO, CRC_HI
    };

    void parseFrame(uint8_t b) {
        switch (fstate_) {
        case FState::STX:
            if (b == kRS485_STX) fstate_ = FState::DST;
            break;
        case FState::DST:
            frameDst_ = b;
            fstate_ = FState::SRC;
            break;
        case FState::SRC:
            frameSrc_ = b;
            fstate_ = FState::LEN_LO;
            break;
        case FState::LEN_LO:
            frameLen_ = b;
            fstate_ = FState::LEN_HI;
            break;
        case FState::LEN_HI:
            frameLen_ |= static_cast<uint16_t>(b) << 8;
            frameRead_ = 0;
            fstate_ = (frameLen_ > 0 && frameLen_ <= sizeof(payloadBuf_))
                       ? FState::PAYLOAD : FState::STX;
            break;
        case FState::PAYLOAD:
            payloadBuf_[frameRead_++] = b;
            if (frameRead_ == frameLen_) fstate_ = FState::CRC_LO;
            break;
        case FState::CRC_LO:
            frameCrc_ = b;
            fstate_ = FState::CRC_HI;
            break;
        case FState::CRC_HI:
            frameCrc_ |= static_cast<uint16_t>(b) << 8;
            fstate_ = FState::STX;
            // Verify CRC and dispatch if addressed to us (or broadcast 0x00).
            if (frameDst_ == busAddr_ || frameDst_ == 0x00) {
                dispatchFrame();
            }
            break;
        }
    }

    void dispatchFrame() {
        if (frameLen_ == 0) return;
        uint8_t msgType = payloadBuf_[0];

        switch (msgType) {
        case kRS485_PROBE:
            // Respond with probe-ack (our bus address + name)
            sendProbeAck();
            break;
        case kRS485_DATA:
            // DCS-BIOS delta — parse and fire callbacks
            parseDcsFrame(payloadBuf_ + 1, frameLen_ - 1);
            break;
        case kRS485_MODE:
            if (frameLen_ >= 2) {
                uint8_t mode = payloadBuf_[1];
                if (mode <= 2) {
                    // Re-use the PC mode handler (same frame semantics)
                    // Build a synthetic AA DE AD 04 [mode] sequence
                    uint8_t synth[5] = { kHL_HDR0, kHL_HDR1, kHL_HDR2, kHL_MODE, mode };
                    for (auto bv : synth) processModeByte(bv, bus_);
                }
            }
            break;
        }
    }

    void sendProbeAck() {
        uint8_t payload[1] = { kRS485_PROBE_ACK };
        sendBusFrame(frameSrc_, payload, 1);
    }

    /**
     * @brief Parse a DCS-BIOS delta frame and fire callbacks for subscribed addresses.
     *
     * Expected format: [sync x4][addr_lo addr_hi len_lo len_hi data...] repeating
     */
    void parseDcsFrame(const uint8_t* data, uint16_t len) {
        uint16_t i = 0;
        // Skip sync bytes
        while (i < len && data[i] == 0x55) i++;
        while (i + 4 <= len) {
            uint16_t addr   = static_cast<uint16_t>(data[i]) | (static_cast<uint16_t>(data[i+1]) << 8);
            uint16_t recLen = static_cast<uint16_t>(data[i+2]) | (static_cast<uint16_t>(data[i+3]) << 8);
            i += 4;
            if (i + recLen > len) break;
            for (uint16_t w = 0; w + 1 < recLen; w += 2) {
                uint16_t val = static_cast<uint16_t>(data[i + w]) | (static_cast<uint16_t>(data[i + w + 1]) << 8);
                uint16_t wordAddr = addr + w;
                if (wantsAddress(wordAddr) && callback_) {
                    callback_(wordAddr, val);
                }
            }
            i += recLen;
        }
    }

    bool wantsAddress(uint16_t addr) const {
        for (uint16_t i = 0; i < subCount_; i++) {
            if (subs_[i].address == 0xFFFF && subs_[i].mask == 0xFFFF) return true;
            if (subs_[i].address == addr) return true;
        }
        return false;
    }

    void sendBusFrame(uint8_t dst, const uint8_t* payload, uint8_t payloadLen) {
        uint8_t header[5] = {
            kRS485_STX, dst, busAddr_,
            static_cast<uint8_t>(payloadLen & 0xFF),
            static_cast<uint8_t>((payloadLen >> 8) & 0xFF)
        };
        // CRC over dst, src, len_lo, len_hi, payload
        uint16_t crc = 0xFFFF;
        for (uint8_t i = 1; i < 5; i++) {
            crc ^= static_cast<uint16_t>(header[i]) << 8;
            for (uint8_t b = 0; b < 8; b++)
                crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1);
        }
        for (uint8_t i = 0; i < payloadLen; i++) {
            crc ^= static_cast<uint16_t>(payload[i]) << 8;
            for (uint8_t b = 0; b < 8; b++)
                crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1);
        }

        digitalWrite(dirPin_, HIGH);
        delayMicroseconds(10);
        bus_.write(header, 5);
        bus_.write(payload, payloadLen);
        bus_.write(static_cast<uint8_t>(crc & 0xFF));
        bus_.write(static_cast<uint8_t>((crc >> 8) & 0xFF));
        bus_.flush();
        delayMicroseconds(10);
        digitalWrite(dirPin_, LOW);
    }

    HardwareSerial&          bus_;
    uint8_t                  dirPin_;
    HornetLinkImport         importer_;
    uint8_t                  busAddr_   = 0;
    const char*              name_      = "";
    const hl_subscription_t* subs_      = nullptr;
    uint16_t                 subCount_  = 0;
    uint8_t                  flags_     = 0;
    hl_state_cb_t            callback_  = nullptr;

    // Frame parser state
    FState   fstate_     = FState::STX;
    uint8_t  frameDst_   = 0;
    uint8_t  frameSrc_   = 0;
    uint16_t frameLen_   = 0;
    uint16_t frameRead_  = 0;
    uint16_t frameCrc_   = 0;
    uint8_t  payloadBuf_[512];
};
