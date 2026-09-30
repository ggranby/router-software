/**
 * @file HnFrame.h
 * @brief Protocol v2 framing: CRC-16, COBS, frame encoder and streaming decoder.
 *
 * Plain C++11 with no Arduino or STL dependency, so the same code runs on a
 * Pro Micro, an ESP32, the PC bridge and the unit tests.
 *
 * Wire format (see docs/PROTOCOL_V2.md):
 * @code
 *   raw   = [ver][dst][src][type][seq][len][payload: len bytes][crc_lo][crc_hi]
 *   wire  = COBS(raw) 0x00
 * @endcode
 * COBS removes every 0x00 from the frame, so 0x00 only ever means "end of
 * frame". A receiver that joins mid-stream, or sees a corrupted byte, is back
 * in step at the next 0x00.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "HnSpec.h"

namespace hn {

/// Largest raw (pre-COBS) frame: header + payload + CRC.
constexpr size_t kMaxRawFrame = kHeaderBytes + kMaxPayload + kCrcBytes;
/// Largest COBS-encoded frame including the trailing 0x00 delimiter.
constexpr size_t kMaxWireFrame = kMaxRawFrame + (kMaxRawFrame / 254) + 1 + 1;

// ── CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF), same as protocol v1 ───────
inline uint16_t crc16Update(uint16_t crc, uint8_t b) {
    crc ^= static_cast<uint16_t>(b) << 8;
    for (uint8_t i = 0; i < 8; i++)
        crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021) : static_cast<uint16_t>(crc << 1);
    return crc;
}

inline uint16_t crc16(const uint8_t* data, size_t len, uint16_t crc = 0xFFFF) {
    for (size_t i = 0; i < len; i++) crc = crc16Update(crc, data[i]);
    return crc;
}

// ── Little-endian helpers ─────────────────────────────────────────────────
inline uint16_t getU16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
inline void putU16(uint8_t* p, uint16_t v) { p[0] = static_cast<uint8_t>(v); p[1] = static_cast<uint8_t>(v >> 8); }
inline uint32_t getU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
inline void putU32(uint8_t* p, uint32_t v) {
    for (uint8_t i = 0; i < 4; i++) p[i] = static_cast<uint8_t>(v >> (8 * i));
}

// ── COBS ─────────────────────────────────────────────────────────────────

/**
 * @brief Streaming COBS encoder into a caller-owned buffer.
 * Call put() for each raw byte, then finish(). ok() is false if @p cap was
 * too small; the buffer contents are then undefined.
 */
class CobsWriter {
public:
    CobsWriter(uint8_t* out, size_t cap) : out_(out), cap_(cap) { start(); }

    void put(uint8_t b) {
        if (b == 0) { closeBlock(); return; }
        emit(b);
        if (++code_ == 0xFF) closeBlock();
    }

    /// Close the last block and append the 0x00 delimiter. Returns bytes written.
    size_t finish() {
        if (codeIdx_ < cap_) out_[codeIdx_] = code_; else ok_ = false;
        if (n_ < cap_) out_[n_++] = 0x00; else ok_ = false;
        return ok_ ? n_ : 0;
    }

    bool ok() const { return ok_; }

private:
    void start() { codeIdx_ = n_; code_ = 1; if (n_ < cap_) n_++; else ok_ = false; }
    void emit(uint8_t b) { if (n_ < cap_) out_[n_++] = b; else ok_ = false; }
    void closeBlock() {
        if (codeIdx_ < cap_) out_[codeIdx_] = code_; else ok_ = false;
        start();
    }

    uint8_t* out_;
    size_t cap_;
    size_t n_ = 0;
    size_t codeIdx_ = 0;
    uint8_t code_ = 1;
    bool ok_ = true;
};

/**
 * @brief Decode a COBS block (without its 0x00 delimiter) in place.
 * @return Decoded length, or -1 if the data is not valid COBS.
 */
inline int cobsDecodeInPlace(uint8_t* buf, size_t len) {
    size_t r = 0, w = 0;
    while (r < len) {
        uint8_t code = buf[r++];
        if (code == 0) return -1;
        for (uint8_t i = 1; i < code; i++) {
            if (r >= len) return -1;
            uint8_t b = buf[r++];
            if (b == 0) return -1;
            buf[w++] = b;
        }
        if (code != 0xFF && r < len) buf[w++] = 0;
    }
    return static_cast<int>(w);
}

// ── Frames ───────────────────────────────────────────────────────────────

/// Decoded frame header.
struct FrameHeader {
    uint8_t version = kProtocolVersion;
    uint8_t dst = ADDR_BROADCAST;
    uint8_t src = ADDR_LINK_NODE;
    uint8_t type = MSG_HEARTBEAT;
    uint8_t seq = 0;
    uint8_t len = 0;
};

/**
 * @brief Encode one frame into @p out (COBS + trailing 0x00).
 * @return Number of wire bytes, or 0 if the payload is too large or @p cap is too small.
 */
inline size_t encodeFrame(const FrameHeader& h, const uint8_t* payload, size_t len,
                          uint8_t* out, size_t cap) {
    if (len > kMaxPayload || (len > 0 && payload == nullptr)) return 0;
    const uint8_t head[kHeaderBytes] = {h.version, h.dst, h.src, h.type, h.seq, static_cast<uint8_t>(len)};
    CobsWriter w(out, cap);
    uint16_t crc = 0xFFFF;
    for (uint8_t b : head) { w.put(b); crc = crc16Update(crc, b); }
    for (size_t i = 0; i < len; i++) { w.put(payload[i]); crc = crc16Update(crc, payload[i]); }
    w.put(static_cast<uint8_t>(crc & 0xFF));
    w.put(static_cast<uint8_t>(crc >> 8));
    return w.finish();
}

/// Receive-side error counters (reported in DIAG).
struct LinkCounters {
    uint16_t frames = 0;
    uint16_t crcErrors = 0;
    uint16_t framingErrors = 0; ///< bad COBS, short frame, length mismatch
    uint16_t versionErrors = 0;
    uint16_t overflows = 0;     ///< frames longer than kMaxWireFrame (rejected early)
};

/**
 * @brief Streaming frame decoder. Feed it every received byte.
 *
 * Uses one kMaxWireFrame buffer and decodes in place. Oversized input is
 * rejected as soon as it exceeds the maximum wire size; the decoder then
 * skips to the next 0x00.
 */
class FrameDecoder {
public:
    enum class Result : uint8_t { None, Frame, Error };

    Result feed(uint8_t b) {
        if (b != 0) {
            if (discarding_) return Result::None;
            if (n_ >= sizeof(buf_)) {
                discarding_ = true;
                n_ = 0;
                counters.overflows++;
                return Result::Error;
            }
            buf_[n_++] = b;
            return Result::None;
        }
        // Delimiter: end of frame.
        if (discarding_) { discarding_ = false; return Result::None; }
        size_t n = n_;
        n_ = 0;
        if (n == 0) return Result::None; // idle delimiter
        int raw = cobsDecodeInPlace(buf_, n);
        if (raw < static_cast<int>(kHeaderBytes + kCrcBytes)) { counters.framingErrors++; return Result::Error; }
        const size_t rawLen = static_cast<size_t>(raw);
        if (buf_[0] != kProtocolVersion) { counters.versionErrors++; return Result::Error; }
        const size_t len = buf_[5];
        if (len > kMaxPayload || rawLen != kHeaderBytes + len + kCrcBytes) {
            counters.framingErrors++;
            return Result::Error;
        }
        const uint16_t want = getU16(buf_ + kHeaderBytes + len);
        if (crc16(buf_, kHeaderBytes + len) != want) { counters.crcErrors++; return Result::Error; }
        header_.version = buf_[0];
        header_.dst = buf_[1];
        header_.src = buf_[2];
        header_.type = buf_[3];
        header_.seq = buf_[4];
        header_.len = static_cast<uint8_t>(len);
        counters.frames++;
        return Result::Frame;
    }

    /// Valid after feed() returned Frame, until the next feed().
    const FrameHeader& header() const { return header_; }
    const uint8_t* payload() const { return buf_ + kHeaderBytes; }

    /// True while bytes of an unfinished frame are buffered (the line is busy).
    bool midFrame() const { return n_ != 0 || discarding_; }

    void reset() { n_ = 0; discarding_ = false; }

    LinkCounters counters;

private:
    uint8_t buf_[kMaxWireFrame];
    size_t n_ = 0;
    bool discarding_ = false;
    FrameHeader header_;
};

/**
 * @brief Sequence-number duplicate filter, one slot per source address.
 *
 * A retried frame carries the same sequence number as the original. accept()
 * returns false for the retry so the input is applied only once.
 * Uses 512 bytes, so it is meant for the bridge and bus masters, not a Pro Micro.
 */
class DuplicateFilter {
public:
    bool accept(uint8_t src, uint8_t seq) {
        const uint16_t tagged = static_cast<uint16_t>(0x100 | seq);
        if (last_[src] == tagged) return false;
        last_[src] = tagged;
        return true;
    }
    void forget(uint8_t src) { last_[src] = 0; }

private:
    uint16_t last_[256] = {};
};

} // namespace hn
