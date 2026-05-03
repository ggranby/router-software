#pragma once
/**
 * @file RS485ProtocolSpec.hpp
 * @brief Hornet Link RS485 sub-bus protocol constants and frame type documentation.
 *
 * @details
 * This header is the single authoritative source for all RS485 frame types,
 * addressing constants, timing parameters, and CRC specification used on the
 * Hornet Link sub-bus (ESP32/Mega 2560 master ↔ Arduino Pro Micro slaves).
 *
 * Including this header from both the PC hub and the firmware library ensures
 * both sides stay in sync.  Arduino sketches should copy or symlink this file
 * into their project tree.
 *
 * ### Bus topology
 * @code
 * PC (USB)
 *  └─ Mega 2560 / ESP32 (master, addr 0x00)
 *      ├─ Pro Micro #1 (slave, addr 0x01)
 *      ├─ Pro Micro #2 (slave, addr 0x02)
 *      └─ ... (up to 0xFE = 254 slaves)
 * @endcode
 *
 * The master is always at address 0x00.  Slaves use addresses 0x01–0xFE.
 * Address 0xFF is reserved (not transmitted).
 * Address 0x00 is the broadcast address when used as the destination.
 *
 * ### Frame wire format (all integers little-endian)
 * @code
 *  [STX:1]  [dst_addr:1]  [src_addr:1]  [payload_len:2 LE]  [payload:N]  [crc16:2 LE]
 *    0xFE       0x01          0x00            0x0006         <data…>        <crc>
 * @endcode
 *
 *  - `STX` is always `0xFE`.
 *  - `dst_addr` = destination slave address; 0x00 = broadcast.
 *  - `src_addr` = sender address; 0x00 = master, 0x01–0xFE = slave.
 *  - `payload_len` is the number of payload bytes (max @ref kRS485MaxPayloadBytes).
 *  - `crc16` covers the header (`dst_addr`, `src_addr`, `payload_len`) and payload.
 *    Algorithm: **CRC-16/CCITT-FALSE** (polynomial 0x1021, init 0xFFFF, no reflect).
 *
 * ### Payload types
 * The payload is one of:
 *  - A Hornet Link sub-protocol frame (probe, delta frame, poll, import flush).
 *  - A DCS-BIOS-compat export byte stream (for legacy slave firmware).
 *
 * ### Half-duplex arbitration
 * The master owns the bus.  It transmits a frame, then releases the TX line
 * and listens for a reply within @ref kRS485ReplyTimeoutMs.  Slaves NEVER
 * transmit unsolicited.
 *
 * ### Timing constants
 * | Constant                    | Value   | Description                                 |
 * |-----------------------------|---------|---------------------------------------------|
 * | kRS485BaudRate              | 250000  | Baud rate (matching Hornet Link USB baud)   |
 * | kRS485ReplyTimeoutMs        | 10      | Max reply wait after master TX              |
 * | kRS485KeepAlivePollMs       | 20      | Max silence before master sends keep-alive  |
 * | kRS485ProbeTimeoutMs        | 300     | Wait for handshake reply from slave         |
 * | kRS485MaxPayloadBytes       | 512     | Maximum RS485 frame payload (bytes)         |
 * | kRS485BroadcastAddr         | 0x00    | Broadcast destination address               |
 * | kRS485MasterAddr            | 0x00    | Master source address                       |
 * | kRS485ReservedAddr          | 0xFF    | Reserved — never transmitted                |
 *
 * @copyright Copyright 2016-2026 Hornet Link contributors.
 *            Licensed under the Apache License, Version 2.0.
 */

#pragma once
#include <cstdint>

namespace dcsbios {

// ─────────────────────────────────────────────────────────────────────────────
// @name RS485 bus addressing
// @{

/// Broadcast destination address — frame is accepted by all slaves.
constexpr uint8_t kRS485BroadcastAddr   = 0x00;
/// Source address used by the bus master in all frames it sends.
constexpr uint8_t kRS485MasterAddr      = 0x00;
/// Reserved address — never appears in valid frames.
constexpr uint8_t kRS485ReservedAddr    = 0xFF;
/// First valid slave address.
constexpr uint8_t kRS485FirstSlaveAddr  = 0x01;
/// Last valid slave address.
constexpr uint8_t kRS485LastSlaveAddr   = 0xFE;
/// Maximum number of slaves on one bus.
constexpr int     kRS485MaxSlaves       = kRS485LastSlaveAddr - kRS485FirstSlaveAddr + 1;

// @}

// ─────────────────────────────────────────────────────────────────────────────
// @name RS485 frame structure
// @{

/// Frame start-of-transmission marker.
constexpr uint8_t kRS485STX = 0xFE;
/// Number of bytes in the fixed frame header (STX + dst + src + len16).
constexpr int     kRS485HeaderBytes = 5;
/// Number of bytes in the CRC trailer.
constexpr int     kRS485CrcBytes    = 2;
/// Maximum payload bytes per frame (avoids excessive master hold time).
constexpr int     kRS485MaxPayloadBytes = 512;
/// Total maximum frame size: header + max payload + CRC.
constexpr int     kRS485MaxFrameBytes =
    kRS485HeaderBytes + kRS485MaxPayloadBytes + kRS485CrcBytes;

// @}

// ─────────────────────────────────────────────────────────────────────────────
// @name RS485 timing
// @{

/// Baud rate for all RS485 bus communication.
constexpr uint32_t kRS485BaudRate = 250000;
/**
 * @brief Maximum milliseconds the master waits for a slave reply after sending.
 *
 * If no reply arrives within this window the master logs a poll-miss and
 * continues to the next slave.  At 250000 baud a 512-byte payload takes ~17 ms
 * to transmit so the reply timeout must be > the worst-case TX time plus
 * the slave's processing time (~2–3 ms on AVR).  10 ms is tight — increase
 * to 20 ms if poll-miss rate is unacceptable on long cable runs.
 */
constexpr int kRS485ReplyTimeoutMs = 10;
/**
 * @brief Maximum silence period before the master sends a keep-alive poll.
 *
 * Bounds switch-input latency during sim-idle periods when no delta frames
 * need to be forwarded.  Default 20 ms → worst-case switch latency 20 ms.
 */
constexpr int kRS485KeepAlivePollMs = 20;
/**
 * @brief Timeout for slave handshake response during bus probing.
 *
 * Longer than kRS485ReplyTimeoutMs because the slave may be mid-init.
 */
constexpr int kRS485ProbeTimeoutMs = 300;

// @}

// ─────────────────────────────────────────────────────────────────────────────
// @name Protocol message types (sub-bus payload first byte)
// @{

/**
 * @brief Sub-bus probe (master → slave): request capability handshake.
 *
 * Payload: `[0xAA][0xDE][0xAD][0x01]` (same 4 bytes as the USB handshake probe).
 * Wrapped in RS485 frame before transmission.
 */
constexpr uint8_t kSubBusMsgProbe       = 0x01;
/**
 * @brief Sub-bus handshake response (slave → master).
 *
 * Payload: `[0xAA][0xDE][0xAD][0x02][flags][name_len][name:N][sub_count:2][subs...]`.
 * Same structure as the USB handshake response.
 */
constexpr uint8_t kSubBusMsgHandshake   = 0x02;
/**
 * @brief Sub-bus handshake ACK (master → slave).
 *
 * Payload: `[0xAA][0xDE][0xAD][0x03]`.
 */
constexpr uint8_t kSubBusMsgAck         = 0x03;
/**
 * @brief Sub-bus mode broadcast (master → slave or PC → master).
 *
 * Payload: `[0xAA][0xDE][0xAD][0x04][mode:1]`.
 * mode: 0=Sim, 1=Preflight, 2=Maintenance.
 */
constexpr uint8_t kSubBusMsgMode        = 0x04;
/**
 * @brief Sub-bus local mode override (slave → master or device → PC).
 *
 * Payload: `[0xAA][0xDE][0xAD][0x05][local_mode:1]`.
 */
constexpr uint8_t kSubBusMsgLocalMode   = 0x05;
/**
 * @brief Sub-bus delta frame (master → slave): compressed state-map update.
 *
 * Payload: DCS-BIOS write records (sync frame + write records), same as USB.
 */
constexpr uint8_t kSubBusMsgDeltaFrame  = 0x06;
/**
 * @brief Sub-bus keep-alive poll (master → slave): "any pending imports?".
 *
 * Payload: empty (header + CRC only).  Slave replies with kSubBusMsgImportFlush.
 */
constexpr uint8_t kSubBusMsgKeepAlive   = 0x07;
/**
 * @brief Sub-bus import flush (slave → master): pending import commands.
 *
 * Payload: zero or more plain-text import lines (`IDENTIFIER SET_STATE v\n`).
 * Sent as a reply to either a delta frame delivery or a keep-alive poll.
 */
constexpr uint8_t kSubBusMsgImportFlush = 0x08;

// @}

// ─────────────────────────────────────────────────────────────────────────────
// @name Mode system values
// @{

/// Hub mode: normal simulation operation.  Import commands forwarded to DCS.
constexpr uint8_t kModeValueSim          = 0;
/// Hub mode: preflight.  Switches tracked, imports not forwarded to simulator.
constexpr uint8_t kModeValuePreflight    = 1;
/// Hub mode: maintenance.  Same as preflight + hub enables diagnostic output.
constexpr uint8_t kModeValueMaintenance  = 2;

// @}

// ─────────────────────────────────────────────────────────────────────────────
// @name CRC-16/CCITT-FALSE helpers
// @{

/**
 * @brief Compute CRC-16/CCITT-FALSE for a byte buffer.
 *
 * @details
 * Parameters: poly=0x1021, init=0xFFFF, no input or output reflection,
 * no final XOR.  This is the same CRC used by DCS-BIOS on its RS485 bus.
 *
 * @param data   Pointer to the data bytes.
 * @param len    Number of bytes to process.
 * @return 16-bit CRC value.
 */
inline uint16_t crc16CcittFalse(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int b = 0; b < 8; ++b) {
            if (crc & 0x8000u) crc = (crc << 1) ^ 0x1021u;
            else                crc <<= 1;
        }
    }
    return crc;
}

// @}

// ─────────────────────────────────────────────────────────────────────────────
// RS485Frame — encode/decode helpers
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Utility functions for encoding and decoding RS485 framed messages.
 */
struct RS485Frame {
    /**
     * @brief Build a complete RS485 frame ready to transmit.
     *
     * @param dst      Destination slave address (use kRS485BroadcastAddr for all).
     * @param src      Source address (kRS485MasterAddr for the master).
     * @param payload  Payload bytes.
     * @param len      Payload length (must be <= kRS485MaxPayloadBytes).
     * @return Raw bytes including STX, header, payload, and CRC.
     */
    static std::vector<uint8_t> encode(uint8_t dst, uint8_t src,
                                        const uint8_t* payload, uint16_t len) {
        std::vector<uint8_t> frame;
        frame.reserve(kRS485HeaderBytes + len + kRS485CrcBytes);

        // Header
        frame.push_back(kRS485STX);
        frame.push_back(dst);
        frame.push_back(src);
        frame.push_back(static_cast<uint8_t>(len & 0xFF));
        frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));

        // Payload
        for (uint16_t i = 0; i < len; ++i) frame.push_back(payload[i]);

        // CRC over dst + src + len16 + payload (everything after STX)
        uint16_t crc = crc16CcittFalse(frame.data() + 1, frame.size() - 1);
        frame.push_back(static_cast<uint8_t>(crc & 0xFF));
        frame.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));

        return frame;
    }

    /**
     * @brief Verify the CRC of a received frame.
     *
     * @param frame  Raw received bytes (starting from STX).
     * @param len    Total frame length.
     * @return True if the CRC matches; false if the frame is corrupted.
     */
    static bool verifyCrc(const uint8_t* frame, size_t len) {
        if (len < static_cast<size_t>(kRS485HeaderBytes + kRS485CrcBytes)) return false;
        // CRC covers bytes 1..(len-3) inclusive
        size_t coveredLen = len - 1 - kRS485CrcBytes;
        uint16_t computed = crc16CcittFalse(frame + 1, coveredLen);
        uint16_t received = frame[len - 2] | (static_cast<uint16_t>(frame[len - 1]) << 8);
        return computed == received;
    }
};

#ifndef ARDUINO
// On the PC side, include <vector> for RS485Frame::encode.
// On Arduino, the caller provides a fixed-size buffer instead.
#include <vector>
#endif

} // namespace dcsbios
