/**
 * @file HnRecords.h
 * @brief Typed payload codecs for protocol v2 messages.
 *
 * Every writer fills a caller-owned buffer and returns false when a record
 * does not fit, so the caller can send the frame and start a new one. Every
 * reader validates lengths and stops (with error() set) at the first
 * malformed record; it never reads past the payload.
 */

#pragma once

#include "HnFrame.h"

namespace hn {

// ── STATE ─────────────────────────────────────────────────────────────────

/// One decoded STATE record.
struct StateRecord {
    uint16_t id = 0;
    uint8_t kind = VALUE_BOOL;  ///< ValueCode
    uint16_t value = 0;         ///< BOOL/POSITION/ANALOG
    const uint8_t* text = nullptr; ///< TEXT bytes (not NUL-terminated)
    uint8_t textLen = 0;
};

class StateWriter {
public:
    StateWriter(uint8_t* buf, size_t cap) : buf_(buf), cap_(cap > kMaxPayload ? kMaxPayload : cap) {}
    bool addBool(uint16_t id, bool on) { return addSmall(id, VALUE_BOOL, on ? 1 : 0); }
    bool addPosition(uint16_t id, uint8_t pos) { return addSmall(id, VALUE_POSITION, pos); }
    bool addAnalog(uint16_t id, uint16_t v) {
        if (n_ + 5 > cap_) return false;
        putU16(buf_ + n_, id); buf_[n_ + 2] = VALUE_ANALOG; putU16(buf_ + n_ + 3, v);
        n_ += 5;
        return true;
    }
    bool addText(uint16_t id, const uint8_t* text, uint8_t len) {
        if (n_ + 4u + len > cap_) return false;
        putU16(buf_ + n_, id); buf_[n_ + 2] = VALUE_TEXT; buf_[n_ + 3] = len;
        if (len) memcpy(buf_ + n_ + 4, text, len);
        n_ += 4u + len;
        return true;
    }
    size_t size() const { return n_; }
    void clear() { n_ = 0; }

private:
    bool addSmall(uint16_t id, uint8_t kind, uint8_t v) {
        if (n_ + 4 > cap_) return false;
        putU16(buf_ + n_, id); buf_[n_ + 2] = kind; buf_[n_ + 3] = v;
        n_ += 4;
        return true;
    }
    uint8_t* buf_;
    size_t cap_;
    size_t n_ = 0;
};

class StateReader {
public:
    StateReader(const uint8_t* p, size_t len) : p_(p), len_(len) {}
    bool next(StateRecord& r) {
        if (err_ || pos_ >= len_) return false;
        if (len_ - pos_ < 4) return fail();
        r.id = getU16(p_ + pos_);
        r.kind = p_[pos_ + 2];
        r.text = nullptr;
        r.textLen = 0;
        switch (r.kind) {
        case VALUE_BOOL:
        case VALUE_POSITION:
            r.value = p_[pos_ + 3];
            pos_ += 4;
            return true;
        case VALUE_ANALOG:
            if (len_ - pos_ < 5) return fail();
            r.value = getU16(p_ + pos_ + 3);
            pos_ += 5;
            return true;
        case VALUE_TEXT: {
            const uint8_t tl = p_[pos_ + 3];
            if (len_ - pos_ - 4 < tl) return fail();
            r.text = p_ + pos_ + 4;
            r.textLen = tl;
            r.value = 0;
            pos_ += 4u + tl;
            return true;
        }
        default:
            return fail();
        }
    }
    bool error() const { return err_; }

private:
    bool fail() { err_ = true; return false; }
    const uint8_t* p_;
    size_t len_;
    size_t pos_ = 0;
    bool err_ = false;
};

// ── INPUT ─────────────────────────────────────────────────────────────────

struct InputRecord {
    uint16_t id = 0;
    uint8_t action = ACTION_SET_POSITION; ///< ActionCode
    uint16_t arg = 0;                     ///< position, analog value, or int16 step count
    int16_t step() const { return static_cast<int16_t>(arg); }
};

constexpr size_t kInputRecordBytes = 5;

inline bool writeInput(uint8_t* buf, size_t cap, size_t& n, const InputRecord& r) {
    if (cap > kMaxPayload) cap = kMaxPayload;
    if (n + kInputRecordBytes > cap) return false;
    putU16(buf + n, r.id); buf[n + 2] = r.action; putU16(buf + n + 3, r.arg);
    n += kInputRecordBytes;
    return true;
}

class InputReader {
public:
    InputReader(const uint8_t* p, size_t len) : p_(p), len_(len), err_(len % kInputRecordBytes != 0) {}
    bool next(InputRecord& r) {
        if (err_ || pos_ + kInputRecordBytes > len_) return false;
        r.id = getU16(p_ + pos_); r.action = p_[pos_ + 2]; r.arg = getU16(p_ + pos_ + 3);
        pos_ += kInputRecordBytes;
        if (r.action > ACTION_ANALOG) { err_ = true; return false; }
        return true;
    }
    bool error() const { return err_; }

private:
    const uint8_t* p_;
    size_t len_;
    size_t pos_ = 0;
    bool err_;
};

// ── SYNC_REPORT ─────────────────────────────────────────────────────────

constexpr uint8_t kSyncLastPage = 0x01;

struct SyncRecord { uint16_t id = 0; uint16_t value = 0; };

class SyncReader {
public:
    SyncReader(const uint8_t* p, size_t len)
        : p_(p), len_(len), err_(len < 1 || (len - 1) % 4 != 0), flags_(len ? p[0] : 0) {}
    bool lastPage() const { return (flags_ & kSyncLastPage) != 0; }
    bool next(SyncRecord& r) {
        if (err_ || pos_ + 4 > len_) return false;
        r.id = getU16(p_ + pos_); r.value = getU16(p_ + pos_ + 2);
        pos_ += 4;
        return true;
    }
    bool error() const { return err_; }

private:
    const uint8_t* p_;
    size_t len_;
    size_t pos_ = 1;
    bool err_;
    uint8_t flags_;
};

// ── HELLO ────────────────────────────────────────────────────────────────

struct Hello {
    uint8_t protoVersion = kProtocolVersion;
    uint8_t role = ROLE_PANEL;
    uint8_t fwMajor = 0;
    uint8_t fwMinor = 0;
    uint32_t catalogHash = 0;
    uint8_t boardId[8] = {};
    char name[kMaxName + 1] = {};
};

/// @return payload length, or 0 if @p cap is too small.
inline size_t writeHello(uint8_t* buf, size_t cap, const Hello& h) {
    size_t nameLen = strlen(h.name);
    if (nameLen > kMaxName) nameLen = kMaxName;
    const size_t need = 17 + nameLen;
    if (need > cap || need > kMaxPayload) return 0;
    buf[0] = h.protoVersion; buf[1] = h.role; buf[2] = h.fwMajor; buf[3] = h.fwMinor;
    putU32(buf + 4, h.catalogHash);
    memcpy(buf + 8, h.boardId, 8);
    buf[16] = static_cast<uint8_t>(nameLen);
    memcpy(buf + 17, h.name, nameLen);
    return need;
}

inline bool readHello(const uint8_t* p, size_t len, Hello& h) {
    if (len < 17) return false;
    const size_t nameLen = p[16];
    if (nameLen > kMaxName || len != 17 + nameLen) return false;
    h.protoVersion = p[0]; h.role = p[1]; h.fwMajor = p[2]; h.fwMinor = p[3];
    h.catalogHash = getU32(p + 4);
    memcpy(h.boardId, p + 8, 8);
    for (size_t i = 0; i < nameLen; i++) {
        const char c = static_cast<char>(p[17 + i]);
        h.name[i] = (c >= 0x20 && c < 0x7F) ? c : '?';
    }
    h.name[nameLen] = '\0';
    return true;
}

// ── DIAG ─────────────────────────────────────────────────────────────────

struct Diag {
    uint16_t crcErrors = 0;
    uint16_t framingErrors = 0;
    uint16_t overflows = 0;
    uint16_t droppedInputs = 0;
    uint8_t lastError = 0;  ///< NackCode of the last rejected request, 0 if none
    uint16_t freeRam = 0;
    char text[32] = {};
};

inline size_t writeDiag(uint8_t* buf, size_t cap, const Diag& d) {
    size_t tl = strlen(d.text);
    if (tl > sizeof(d.text) - 1) tl = sizeof(d.text) - 1;
    const size_t need = 12 + tl;
    if (need > cap || need > kMaxPayload) return 0;
    putU16(buf, d.crcErrors); putU16(buf + 2, d.framingErrors); putU16(buf + 4, d.overflows);
    putU16(buf + 6, d.droppedInputs); buf[8] = d.lastError; putU16(buf + 9, d.freeRam);
    buf[11] = static_cast<uint8_t>(tl);
    memcpy(buf + 12, d.text, tl);
    return need;
}

inline bool readDiag(const uint8_t* p, size_t len, Diag& d) {
    if (len < 12) return false;
    const size_t tl = p[11];
    if (tl > sizeof(d.text) - 1 || len != 12 + tl) return false;
    d.crcErrors = getU16(p); d.framingErrors = getU16(p + 2); d.overflows = getU16(p + 4);
    d.droppedInputs = getU16(p + 6); d.lastError = p[8]; d.freeRam = getU16(p + 9);
    for (size_t i = 0; i < tl; i++) {
        const char c = static_cast<char>(p[12 + i]);
        d.text[i] = (c >= 0x20 && c < 0x7F) ? c : '?';
    }
    d.text[tl] = '\0';
    return true;
}

// ── DESCRIBE ──────────────────────────────────────────────────────────────

/// Records per DESCRIBE page: (kMaxPayload - 2) / 3.
constexpr uint8_t kDescribePerPage = static_cast<uint8_t>((kMaxPayload - 2) / 3);

struct DescribeRecord { uint16_t id = 0; uint8_t role = DESCRIBE_ROLE_INPUT; };

class DescribeReader {
public:
    DescribeReader(const uint8_t* p, size_t len)
        : p_(p), len_(len), err_(len < 2 || (len - 2) % 3 != 0) {}
    uint8_t page() const { return len_ >= 1 ? p_[0] : 0; }
    uint8_t pageCount() const { return len_ >= 2 ? p_[1] : 0; }
    bool next(DescribeRecord& r) {
        if (err_ || pos_ + 3 > len_) return false;
        r.id = getU16(p_ + pos_); r.role = p_[pos_ + 2];
        pos_ += 3;
        return true;
    }
    bool error() const { return err_; }

private:
    const uint8_t* p_;
    size_t len_;
    size_t pos_ = 2;
    bool err_;
};

} // namespace hn
