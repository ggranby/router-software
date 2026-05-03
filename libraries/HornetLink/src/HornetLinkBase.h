/**
 * @file HornetLinkBase.h
 * @brief Shared constants, flag definitions, frame builder helpers,
 *        and the hl_subscription_t struct used by all HornetLink classes.
 */

#pragma once
#include <Arduino.h>

// ── Capability flags (byte 5 of the pong frame) ──────────
#define HL_FLAG_RS485_MASTER 0x01 ///< This device drives an RS-485 slave bus
#define HL_FLAG_RS485_SLAVE  0x02 ///< This device is managed by an RS-485 master
#define HL_FLAG_BIDIR        0x04 ///< This device can send import commands to DCS

// ── Handshake frame bytes ─────────────────────────────────
static constexpr uint8_t kHL_HDR0  = 0xAA;
static constexpr uint8_t kHL_HDR1  = 0xDE;
static constexpr uint8_t kHL_HDR2  = 0xAD;
static constexpr uint8_t kHL_PING  = 0x01; ///< PC → device: probe
static constexpr uint8_t kHL_PONG  = 0x02; ///< device → PC: capability response
static constexpr uint8_t kHL_ACK   = 0x03; ///< PC → device: negotiation complete
static constexpr uint8_t kHL_MODE  = 0x04; ///< PC → device: mode change
static constexpr uint8_t kHL_MACK  = 0x05; ///< device → PC: local mode acknowledge

// ── RS-485 sub-bus constants ──────────────────────────────
static constexpr uint8_t  kRS485_STX     = 0xFE; ///< Sub-bus frame start byte
static constexpr uint32_t kRS485_BAUD    = 250000UL;
static constexpr uint8_t  kRS485_PROBE   = 0x10; ///< Master → slave: are you there?
static constexpr uint8_t  kRS485_PROBE_ACK = 0x11; ///< Slave → master: I am here
static constexpr uint8_t  kRS485_DATA    = 0x20; ///< Master → slave: DCS-BIOS delta
static constexpr uint8_t  kRS485_IMPORT  = 0x30; ///< Slave → master: import line
static constexpr uint8_t  kRS485_MODE    = 0x40; ///< Master → slave: mode change
static constexpr uint8_t  kRS485_MODE_ACK= 0x41; ///< Slave → master: mode ack

#ifndef ARRAYSIZE
  #define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif

// ── Subscription entry ────────────────────────────────────
/**
 * @brief One DCS-BIOS field subscription.
 *
 * The wildcard entry { 0xFFFF, 0xFFFF, 0 } means "send me everything".
 */
struct hl_subscription_t {
    uint16_t address; ///< Even byte address of the 16-bit word
    uint16_t mask;    ///< Bitmask isolating the field in the word
    uint8_t  shift;   ///< Right-shift to extract the raw integer value
};

// ── CRC-16/CCITT-FALSE ───────────────────────────────────
/**
 * @brief Compute CRC-16/CCITT-FALSE over a byte buffer.
 * @param data  Pointer to data bytes.
 * @param len   Number of bytes.
 * @return      16-bit CRC (poly=0x1021, init=0xFFFF).
 */
inline uint16_t hl_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (uint8_t b = 0; b < 8; b++)
            crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1);
    }
    return crc;
}

// ── Pong frame builder ────────────────────────────────────
/**
 * @brief Write a capability pong frame to @p stream.
 *
 * @param stream       Destination serial port.
 * @param flags        Capability flags (HL_FLAG_*).
 * @param name         Device name string.
 * @param subs         Subscription array.
 * @param subCount     Number of subscriptions.
 */
inline void hl_sendPong(Stream& stream,
                        uint8_t flags,
                        const char* name,
                        const hl_subscription_t* subs,
                        uint16_t subCount)
{
    uint8_t nameLen = static_cast<uint8_t>(strlen(name));
    stream.write(kHL_HDR0); stream.write(kHL_HDR1); stream.write(kHL_HDR2);
    stream.write(kHL_PONG);
    stream.write(flags);
    stream.write(nameLen);
    stream.write(reinterpret_cast<const uint8_t*>(name), nameLen);
    stream.write(static_cast<uint8_t>(subCount & 0xFF));
    stream.write(static_cast<uint8_t>((subCount >> 8) & 0xFF));
    for (uint16_t i = 0; i < subCount; i++) {
        stream.write(static_cast<uint8_t>(subs[i].address & 0xFF));
        stream.write(static_cast<uint8_t>((subs[i].address >> 8) & 0xFF));
        stream.write(static_cast<uint8_t>(subs[i].mask & 0xFF));
        stream.write(static_cast<uint8_t>((subs[i].mask >> 8) & 0xFF));
        stream.write(subs[i].shift);
    }
}
