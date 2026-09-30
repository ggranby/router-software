/**
 * @file HnMaster.h
 * @brief Protocol v2 RS-485 bus master (relay between the PC and the slaves).
 *
 * Bus discipline (docs/PROTOCOL_V2.md, "Bus discipline"):
 *  - Only the master starts a transmission. A slave speaks only after a POLL
 *    addressed to it, and answers with exactly one frame (POLL_EMPTY if idle).
 *  - Sim state from the bridge is broadcast once; every slave filters locally.
 *  - INPUT from a slave is acknowledged on the bus by the master (hop-by-hop),
 *    de-duplicated by sequence number, then forwarded to the bridge with the
 *    slave's original header, so the bridge sees who sent it.
 *  - A slave that misses kTimingMissedPollsBeforeDrop polls is reported as
 *    DROP and then polled slowly. When it answers again it is reported as JOIN
 *    and asked for a fresh HELLO, so the bridge can resync it.
 *
 * Needs about 2 KB of RAM: fine on a Mega 2560, ESP32 or Giga. A Pro Micro
 * should be a slave or a USB panel, not a bus master.
 */

#pragma once

#include "HnNode.h"

namespace hn {

struct SlaveStatus {
    uint8_t address = 0;
    bool online = false;
    uint8_t missed = 0;
    uint16_t lastInputSeq = 0; ///< 0x100 | seq of the last forwarded INPUT, 0 = none
    uint32_t lastInputMs = 0;
    uint32_t lastPollMs = 0;
    uint32_t boardHash = 0;
    bool helloThisWindow = false;
    uint8_t pendingAddress = 0;
    uint8_t pendingAddressSeq = 0;
};

struct MasterCounters {
    uint16_t polls = 0;
    uint16_t timeouts = 0;
    uint16_t duplicateInputs = 0;
    uint16_t queueOverflows = 0;
    uint16_t conflicts = 0;
};

class BusMaster {
public:
    static constexpr uint8_t kMaxSlaves = 32;
    static constexpr uint8_t kQueueFrames = 6;
    static constexpr uint16_t kOfflinePollMs = 1000;
    static constexpr uint16_t kInputRetryWindowMs = 1000;

    BusMaster(Port& upstream, Port& bus) : up_(upstream), bus_(bus) {}

    /// Add a fixed bus address (1-239). Returns false if the list is full or invalid.
    bool addSlave(uint8_t address) {
        if (address < ADDR_FIRST_SLAVE || address > ADDR_LAST_SLAVE) return false;
        if (find(address)) return true;
        if (slaveCount_ >= kMaxSlaves) return false;
        slaves_[slaveCount_] = SlaveStatus();
        slaves_[slaveCount_].address = address;
        slaves_[slaveCount_].lastPollMs = now_ - kOfflinePollMs;
        slaveCount_++;
        return true;
    }

    /// Periodic broadcast "who's there" so boards not in the fixed list are found too.
    void enableDiscovery(bool on) { discovery_ = on; }

    void setIdentity(const char* name, uint8_t fwMajor, uint8_t fwMinor, const uint8_t boardId[8]) {
        name_ = name;
        fwMajor_ = fwMajor;
        fwMinor_ = fwMinor;
        if (boardId) memcpy(boardId_, boardId, 8);
    }

    void update(uint32_t nowMs) {
        now_ = nowMs;
        int c;
        while ((c = up_.read()) >= 0)
            if (upDec_.feed(static_cast<uint8_t>(c)) == FrameDecoder::Result::Frame) handleUpstream();
        while ((c = bus_.read()) >= 0)
            if (busDec_.feed(static_cast<uint8_t>(c)) == FrameDecoder::Result::Frame) handleBus();
        runBus();
    }

    uint8_t slaveCount() const { return slaveCount_; }
    const SlaveStatus& slave(uint8_t i) const { return slaves_[i]; }
    const MasterCounters& counters() const { return counters_; }
    const LinkCounters& busCounters() const { return busDec_.counters; }
    const LinkCounters& usbCounters() const { return upDec_.counters; }

private:
    enum class BusState : uint8_t { Idle, AwaitReply, Discover };

    // ── Bus side ─────────────────────────────────────────────────────────
    void runBus() {
        if (state_ == BusState::AwaitReply) {
            const bool late = static_cast<int32_t>(now_ - deadline_) >= 0;
            const bool hardLate = static_cast<int32_t>(now_ - deadline_) >= static_cast<int32_t>(kTimingReplyTimeoutMs);
            if (!late || (busDec_.midFrame() && !hardLate)) return;
            busDec_.reset();
            counters_.timeouts++;
            if (SlaveStatus* s = find(awaiting_)) missed(*s);
            state_ = BusState::Idle;
        }
        if (state_ == BusState::Discover) {
            if (static_cast<int32_t>(now_ - deadline_) < 0) return;
            state_ = BusState::Idle;
        }
        // Idle: drain queued downstream frames first (state, mode, requests, ACKs).
        while (qCount_) {
            const QueuedFrame& f = queue_[qHead_];
            transmit(f.bytes, f.len);
            qHead_ = static_cast<uint8_t>((qHead_ + 1) % kQueueFrames);
            qCount_--;
        }
        if (discovery_ && (now_ - lastDiscover_) >= kTimingDiscoverIntervalMs) {
            lastDiscover_ = now_;
            for (uint8_t i = 0; i < slaveCount_; i++) slaves_[i].helloThisWindow = false;
            const uint8_t p[2] = {static_cast<uint8_t>(kTimingDiscoverSlots), static_cast<uint8_t>(kTimingDiscoverSlotMs)};
            sendBus(ADDR_BROADCAST, MSG_DISCOVER, seq_++, p, 2);
            state_ = BusState::Discover;
            deadline_ = now_ + kTimingDiscoverSlots * kTimingDiscoverSlotMs + kTimingReplyTimeoutMs;
            return;
        }
        pollNext();
    }

    void pollNext() {
        for (uint8_t tries = 0; tries < slaveCount_; tries++) {
            cursor_ = static_cast<uint8_t>((cursor_ + 1) % slaveCount_);
            SlaveStatus& s = slaves_[cursor_];
            if (!s.online && (now_ - s.lastPollMs) < kOfflinePollMs) continue;
            s.lastPollMs = now_;
            counters_.polls++;
            sendBus(s.address, MSG_POLL, seq_++, nullptr, 0);
            awaiting_ = s.address;
            deadline_ = now_ + kTimingReplyTimeoutMs;
            state_ = BusState::AwaitReply;
            return;
        }
    }

    void missed(SlaveStatus& s) {
        if (!s.online) return;
        if (++s.missed >= kTimingMissedPollsBeforeDrop) {
            s.online = false;
            s.lastInputSeq = 0;
            busEvent(BUS_DROP, s.address);
        }
    }

    void handleBus() {
        const FrameHeader& h = busDec_.header();
        if (h.dst != ADDR_LINK_NODE) return;
        const uint8_t* p = busDec_.payload();
        if (state_ == BusState::AwaitReply && h.src == awaiting_) state_ = BusState::Idle;

        SlaveStatus* s = find(h.src);
        if (!s && discovery_ && h.type == MSG_HELLO && addSlave(h.src)) s = find(h.src);
        if (!s) {
            // e.g. a HELLO from an unaddressed board (src 0xFF): let the bridge see it.
            if (h.type == MSG_HELLO) forwardUp(h, p);
            return;
        }
        s->missed = 0;
        if (!s->online) {
            s->online = true;
            busEvent(BUS_JOIN, s->address);
            if (h.type != MSG_HELLO) queueDown(s->address, MSG_HELLO_REQUEST, seq_++, nullptr, 0);
        }
        switch (h.type) {
        case MSG_POLL_EMPTY: return;
        case MSG_INPUT: {
            const uint8_t ack[1] = {h.seq};
            queueDown(s->address, MSG_ACK, seq_++, ack, 1);
            const uint16_t tagged = static_cast<uint16_t>(0x100 | h.seq);
            if (s->lastInputSeq == tagged && now_ - s->lastInputMs < kInputRetryWindowMs) {
                counters_.duplicateInputs++;
                return;
            }
            s->lastInputSeq = tagged;
            s->lastInputMs = now_;
            forwardUp(h, p);
            return;
        }
        case MSG_ACK:
            if (s->pendingAddress && h.len >= 1 && p[0] == s->pendingAddressSeq) {
                s->address = s->pendingAddress;
                s->pendingAddress = s->pendingAddressSeq = 0;
            }
            forwardUp(h, p);
            return;
        case MSG_HELLO: {
            const uint32_t hash = h.len >= 17 ? crc16(p + 8, 8) | (static_cast<uint32_t>(crc16(p + 8, 8, 0x1D0F)) << 16) : 0;
            if (state_ == BusState::Discover && s->helloThisWindow && hash != s->boardHash) {
                counters_.conflicts++;
                busEvent(BUS_CONFLICT, s->address);
            }
            s->helloThisWindow = state_ == BusState::Discover;
            s->boardHash = hash;
            forwardUp(h, p);
            return;
        }
        default:
            forwardUp(h, p);
            return;
        }
    }

    // ── USB side ─────────────────────────────────────────────────────────
    void handleUpstream() {
        const FrameHeader& h = upDec_.header();
        const uint8_t* p = upDec_.payload();
        if (h.dst == ADDR_LINK_NODE) { handleLocal(h, p); return; }
        if (h.dst == ADDR_BROADCAST) {
            if (h.type == MSG_HEARTBEAT) return;
            if (h.type == MSG_HELLO_REQUEST) handleLocal(h, p);
            queueDown(h.dst, h.type, h.seq, p, h.len, h.src);
            return;
        }
        if (h.dst >= ADDR_FIRST_SLAVE && h.dst <= ADDR_LAST_SLAVE) {
            if (h.type == MSG_ACK) return; // the master already acknowledged on the bus
            if (h.type == MSG_CONFIG && h.len >= 2 && p[0] == CFG_BUS_ADDRESS &&
                p[1] >= ADDR_FIRST_SLAVE && p[1] <= ADDR_LAST_SLAVE) {
                if (SlaveStatus* s = find(h.dst)) {
                    s->pendingAddress = p[1];
                    s->pendingAddressSeq = h.seq;
                }
            }
            queueDown(h.dst, h.type, h.seq, p, h.len, h.src);
        }
    }

    void handleLocal(const FrameHeader& h, const uint8_t* p) {
        switch (h.type) {
        case MSG_HELLO_REQUEST: {
            Hello hello;
            hello.role = ROLE_BUS_MASTER;
            hello.fwMajor = fwMajor_;
            hello.fwMinor = fwMinor_;
            memcpy(hello.boardId, boardId_, 8);
            strncpy(hello.name, name_ ? name_ : "BusMaster", kMaxName);
            hello.name[kMaxName] = '\0';
            sendUp(MSG_HELLO, scratch_, writeHello(scratch_, kMaxPayload, hello));
            // The master's handshake lists its slaves: one JOIN per online slave.
            for (uint8_t i = 0; i < slaveCount_; i++)
                if (slaves_[i].online) busEvent(BUS_JOIN, slaves_[i].address);
            return;
        }
        case MSG_DESCRIBE_REQUEST: {
            const uint8_t d[2] = {0, 1};
            sendUp(MSG_DESCRIBE, d, 2);
            return;
        }
        case MSG_DIAG_REQUEST: {
            Diag d;
            d.crcErrors = static_cast<uint16_t>(busDec_.counters.crcErrors + upDec_.counters.crcErrors);
            d.framingErrors = static_cast<uint16_t>(busDec_.counters.framingErrors + upDec_.counters.framingErrors);
            d.overflows = static_cast<uint16_t>(busDec_.counters.overflows + upDec_.counters.overflows + counters_.queueOverflows);
            d.droppedInputs = 0; // the master never drops inputs; duplicates are retries it already forwarded
            uint8_t online = 0;
            for (uint8_t i = 0; i < slaveCount_; i++) online = static_cast<uint8_t>(online + (slaves_[i].online ? 1 : 0));
            snprintfSlaves(d.text, sizeof(d.text), online);
            sendUp(MSG_DIAG, scratch_, writeDiag(scratch_, kMaxPayload, d));
            return;
        }
        case MSG_MODE:
            if (h.len >= 1) sendUp(MSG_MODE_ACK, p, 1);
            return;
        case MSG_CONFIG: {
            const uint8_t n[4] = {h.seq, NACK_UNSUPPORTED, 0, 0};
            sendUp(MSG_NACK, n, 4);
            return;
        }
        default:
            return;
        }
    }

    static void snprintfSlaves(char* out, size_t cap, uint8_t online) {
        // "slaves online N" without pulling in printf on AVR.
        const char prefix[] = "slaves online ";
        size_t i = 0;
        for (; prefix[i] && i + 1 < cap; i++) out[i] = prefix[i];
        char digits[4];
        uint8_t nd = 0;
        do { digits[nd++] = static_cast<char>('0' + online % 10); online /= 10; } while (online && nd < 3);
        while (nd && i + 1 < cap) out[i++] = digits[--nd];
        out[i] = '\0';
    }

    void busEvent(uint8_t event, uint8_t address) {
        const uint8_t e[2] = {event, address};
        sendUp(MSG_BUS_EVENT, e, 2);
    }

    // ── Frame I/O ────────────────────────────────────────────────────────
    struct QueuedFrame {
        uint8_t len;
        uint8_t bytes[kMaxWireFrame];
    };

    void queueDown(uint8_t dst, uint8_t type, uint8_t seq, const uint8_t* p, size_t n,
                   uint8_t src = ADDR_LINK_NODE) {
        if (qCount_ >= kQueueFrames) { counters_.queueOverflows++; return; }
        QueuedFrame& f = queue_[(qHead_ + qCount_) % kQueueFrames];
        FrameHeader h;
        h.dst = dst; h.src = src; h.type = type; h.seq = seq;
        const size_t w = encodeFrame(h, p, n, f.bytes, sizeof(f.bytes));
        if (!w) return;
        f.len = static_cast<uint8_t>(w);
        qCount_++;
    }

    void sendBus(uint8_t dst, uint8_t type, uint8_t seq, const uint8_t* p, size_t n) {
        FrameHeader h;
        h.dst = dst; h.src = ADDR_LINK_NODE; h.type = type; h.seq = seq;
        const size_t w = encodeFrame(h, p, n, wire_, sizeof(wire_));
        if (w) transmit(wire_, w);
    }

    void transmit(const uint8_t* bytes, size_t len) {
        bus_.beginTransmit();
        bus_.write(bytes, len);
        bus_.endTransmit();
    }

    void forwardUp(const FrameHeader& h, const uint8_t* p) {
        const size_t w = encodeFrame(h, p, h.len, wire_, sizeof(wire_));
        if (w) up_.write(wire_, w);
    }

    void sendUp(uint8_t type, const uint8_t* p, size_t n) {
        FrameHeader h;
        h.dst = ADDR_BRIDGE; h.src = ADDR_LINK_NODE; h.type = type; h.seq = seq_++;
        h.len = static_cast<uint8_t>(n);
        forwardUp(h, p);
    }

    SlaveStatus* find(uint8_t address) {
        for (uint8_t i = 0; i < slaveCount_; i++)
            if (slaves_[i].address == address) return &slaves_[i];
        return nullptr;
    }

    Port& up_;
    Port& bus_;
    FrameDecoder upDec_;
    FrameDecoder busDec_;
    SlaveStatus slaves_[kMaxSlaves];
    uint8_t slaveCount_ = 0;
    uint8_t cursor_ = 0;
    BusState state_ = BusState::Idle;
    uint8_t awaiting_ = 0;
    uint32_t deadline_ = 0;
    uint32_t now_ = 0;
    uint32_t lastDiscover_ = 0;
    bool discovery_ = false;
    uint8_t seq_ = 0;
    MasterCounters counters_;

    QueuedFrame queue_[kQueueFrames];
    uint8_t qHead_ = 0, qCount_ = 0;

    const char* name_ = nullptr;
    uint8_t fwMajor_ = 0, fwMinor_ = 0;
    uint8_t boardId_[8] = {};
    uint8_t scratch_[kMaxPayload];
    uint8_t wire_[kMaxWireFrame];
};

} // namespace hn
