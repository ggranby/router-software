/**
 * @file HnNode.h
 * @brief Protocol v2 node engine: one panel board, on USB or on the RS-485 bus.
 *
 * The engine is transport-neutral. The same object runs:
 *  - over USB, where it may send whenever it has something queued, and
 *  - as an RS-485 slave, where it only transmits one frame in reply to a POLL
 *    addressed to it (or a HELLO in a random DISCOVER slot). This is what keeps
 *    slaves from talking over each other on the half-duplex bus.
 *
 * The sketch-facing layer (Hornet::Panel) implements NodeHandler. Everything
 * here is plain C++11 and is exercised by the host-side bus simulation tests.
 */

#pragma once

#include "HnRecords.h"

namespace hn {

/// Byte stream the protocol runs over (a HardwareSerial, a USB CDC port, a test pipe).
// No virtual destructors: these objects are never deleted through a base
// pointer, and a virtual destructor would pull operator delete into AVR builds.
class Port {
public:
    /// Next received byte, or -1 if none is waiting.
    virtual int read() = 0;
    virtual void write(const uint8_t* data, size_t len) = 0;
    /// Called before a frame is written: enable the RS-485 driver.
    virtual void beginTransmit() {}
    /// Called after a frame is written: wait for the last bit, release the driver.
    virtual void endTransmit() {}

protected:
    ~Port() {}
};

/// What the node engine needs from the application.
class NodeHandler {
public:
    virtual void fillHello(Hello& h) = 0;
    /// One sim value arrived. Ignore ids you don't display (local filtering).
    virtual void onState(const StateRecord&) {}
    /// Returns the mode actually applied.
    virtual uint8_t onMode(uint8_t mode) { return mode; }
    /// CONFIG other than BUS_ADDRESS. Return 0 on success or a NACK_* reason.
    virtual uint8_t onConfig(const uint8_t*, size_t) { return NACK_UNSUPPORTED; }
    virtual void onNack(uint8_t /*reason*/, uint16_t /*controlId*/) {}
    virtual void onBusAddressChanged(uint8_t /*address*/) {}
    virtual uint16_t describeCount() { return 0; }
    virtual DescribeRecord describeItem(uint16_t) { return DescribeRecord(); }
    virtual uint16_t syncCount() { return 0; }
    virtual SyncRecord syncItem(uint16_t) { return SyncRecord(); }
    /// Fill a SUBSCRIBE payload. Default: every output.
    virtual size_t writeSubscribe(uint8_t* buf, size_t cap) {
        if (cap < 1) return 0;
        buf[0] = SUBSCRIBE_ALL;
        return 1;
    }
    virtual void fillDiag(Diag&) {}

protected:
    ~NodeHandler() {}
};

enum class Transport : uint8_t { Usb, Bus };

/// Engine tuning. Defaults match docs/PROTOCOL_V2.md.
struct NodeTiming {
    uint16_t usbRetryMs = 100;   ///< USB: resend an unacknowledged INPUT after this long
    uint8_t maxRetries = 3;      ///< then drop it and count it in DIAG
};

class Node {
public:
    static constexpr uint8_t kInputQueue = 16;

    Node(Port& port, NodeHandler& handler) : port_(port), handler_(handler) {}

    /// Standalone panel on USB. Frames carry src = LINK_NODE.
    void beginUsb() { transport_ = Transport::Usb; address_ = ADDR_LINK_NODE; requestHello(); }

    /// RS-485 slave with a fixed bus address (1-239).
    void beginBus(uint8_t address) {
        transport_ = Transport::Bus;
        address_ = address;
        rng_ ^= address;
    }

    /// Mix board identity into the discovery back-off so two boards pick different slots.
    void seedRandom(uint32_t seed) { rng_ ^= seed ? seed : 1; }

    uint8_t address() const { return address_; }
    Transport transport() const { return transport_; }
    NodeTiming timing;

    /// Queue one input event. Repeated SET_POSITION / ANALOG events for the same
    /// control that have not been sent yet are merged (keeps a noisy pot from
    /// filling the queue). Returns false if the queue is full.
    bool queueInput(const InputRecord& r) {
        if (r.action == ACTION_SET_POSITION || r.action == ACTION_ANALOG) {
            for (uint8_t i = inFlight_; i < inCount_; i++) {
                InputRecord& q = inputs_[(inHead_ + i) % kInputQueue];
                if (q.id == r.id && q.action == r.action) { q.arg = r.arg; return true; }
            }
        }
        if (inCount_ >= kInputQueue) { droppedInputs_++; return false; }
        inputs_[(inHead_ + inCount_) % kInputQueue] = r;
        inCount_++;
        return true;
    }

    /// Ask the engine to send a SYNC_REPORT (all physical positions).
    void requestSync() { syncDue_ = true; syncCursor_ = 0; }
    /// Ask the engine to send HELLO + SUBSCRIBE + SYNC_REPORT (what happens on connect).
    void requestHello() { helloDue_ = true; subscribeDue_ = true; requestSync(); }

    /// Call often from loop(). Reads the port, answers polls, handles retries.
    void update(uint32_t nowMs) {
        now_ = nowMs;
        int c;
        while ((c = port_.read()) >= 0) {
            if (decoder_.feed(static_cast<uint8_t>(c)) == FrameDecoder::Result::Frame) handleFrame();
        }
        if (transport_ == Transport::Usb) {
            sendNext(false);
        } else if (discoverPending_ && static_cast<int32_t>(now_ - discoverAt_) >= 0 && !decoder_.midFrame()) {
            discoverPending_ = false;
            sendHello();
        }
    }

    const LinkCounters& counters() const { return decoder_.counters; }
    uint16_t droppedInputs() const { return droppedInputs_; }
    uint8_t lastError() const { return lastError_; }
    bool inputsPending() const { return inCount_ != 0; }

private:
    // ── Receive ──────────────────────────────────────────────────────────
    void handleFrame() {
        const FrameHeader& h = decoder_.header();
        if (h.dst != address_ && h.dst != ADDR_BROADCAST) return;
        const uint8_t* p = decoder_.payload();
        const size_t n = h.len;
        switch (h.type) {
        case MSG_POLL:
            if (transport_ == Transport::Bus && h.dst == address_) {
                lastPolled_ = now_;
                everPolled_ = true;
                if (!sendNext(true)) sendSimple(MSG_POLL_EMPTY, nullptr, 0);
            }
            break;
        case MSG_DISCOVER:
            if (transport_ == Transport::Bus && n >= 2 && recentlyUnpolled()) {
                const uint8_t slots = p[0] ? p[0] : 1;
                discoverAt_ = now_ + (nextRandom() % slots) * p[1] + 1;
                discoverPending_ = true;
            }
            break;
        case MSG_HELLO_REQUEST: requestHello(); break;
        case MSG_DESCRIBE_REQUEST:
            describeDue_ = true;
            describePage_ = n >= 1 ? p[0] : 0;
            break;
        case MSG_STATE: {
            StateReader rd(p, n);
            StateRecord r;
            while (rd.next(r)) handler_.onState(r);
            if (rd.error()) lastError_ = NACK_BAD_FRAME;
            break;
        }
        case MSG_SYNC_REQUEST: requestSync(); break;
        case MSG_MODE:
            if (n >= 1) {
                reply_[0] = handler_.onMode(p[0]);
                queueReply(MSG_MODE_ACK, 1);
            }
            break;
        case MSG_ACK:
            if (n >= 1 && inFlight_ && p[0] == inFlightSeq_) completeInFlight();
            break;
        case MSG_NACK:
            if (n >= 4) {
                lastError_ = p[1];
                handler_.onNack(p[1], getU16(p + 2));
                if (inFlight_ && p[0] == inFlightSeq_) completeInFlight();
            }
            break;
        case MSG_DIAG_REQUEST: diagDue_ = true; break;
        case MSG_CONFIG: handleConfig(h, p, n); break;
        default: break;
        }
    }

    void handleConfig(const FrameHeader& h, const uint8_t* p, size_t n) {
        if (h.dst == ADDR_BROADCAST) return; // settings are always addressed
        uint8_t reason = 0;
        if (n >= 2 && p[0] == CFG_BUS_ADDRESS) {
            if (transport_ != Transport::Bus || p[1] < ADDR_FIRST_SLAVE || p[1] > ADDR_LAST_SLAVE) reason = NACK_BAD_VALUE;
            else pendingAddress_ = p[1]; // applied after the ACK went out from the old address
        } else if (n >= 1) {
            reason = handler_.onConfig(p, n);
        } else {
            reason = NACK_BAD_FRAME;
        }
        if (reason) lastError_ = reason;
        nackOrAck(h.seq, reason, 0);
    }

    void nackOrAck(uint8_t seq, uint8_t reason, uint16_t id) {
        reply_[0] = seq;
        if (!reason) { queueReply(MSG_ACK, 1); return; }
        reply_[1] = reason;
        putU16(reply_ + 2, id);
        queueReply(MSG_NACK, 4);
    }

    void queueReply(uint8_t type, uint8_t len) { replyType_ = type; replyLen_ = len; }

    void completeInFlight() {
        inHead_ = static_cast<uint8_t>((inHead_ + inFlight_) % kInputQueue);
        inCount_ = static_cast<uint8_t>(inCount_ - inFlight_);
        inFlight_ = 0;
        retries_ = 0;
    }

    bool recentlyUnpolled() const { return !everPolled_ || (now_ - lastPolled_) > 2000u; }

    uint32_t nextRandom() {
        rng_ = rng_ * 1103515245u + 12345u;
        return (rng_ >> 16) & 0x7FFF;
    }

    // ── Transmit ──────────────────────────────────────────────────────────
    /// Send the most important pending frame. Returns false if nothing was due.
    bool sendNext(bool polled) {
        if (replyType_) {
            const uint8_t t = replyType_;
            replyType_ = 0;
            sendSimple(t, reply_, replyLen_);
            if (pendingAddress_) {
                address_ = pendingAddress_;
                pendingAddress_ = 0;
                handler_.onBusAddressChanged(address_);
            }
            return true;
        }
        if (helloDue_) { helloDue_ = false; sendHello(); return true; }
        if (inFlight_) {
            const bool due = polled || (now_ - inFlightSentAt_) >= timing.usbRetryMs;
            if (due) {
                if (retries_ >= timing.maxRetries) {
                    droppedInputs_ = static_cast<uint16_t>(droppedInputs_ + inFlight_);
                    completeInFlight();
                } else {
                    retries_++;
                    sendInputs(inFlightSeq_);
                    return true;
                }
            }
        }
        if (subscribeDue_) {
            subscribeDue_ = false;
            sendSimple(MSG_SUBSCRIBE, scratch_, handler_.writeSubscribe(scratch_, kMaxPayload));
            return true;
        }
        if (describeDue_) { describeDue_ = false; sendDescribe(); return true; }
        if (diagDue_) { diagDue_ = false; sendDiag(); return true; }
        if (syncDue_) { sendSyncPage(); return true; }
        if (inCount_ && !inFlight_) {
            inFlight_ = inCount_ > kMaxPayload / kInputRecordBytes ? kMaxPayload / kInputRecordBytes : inCount_;
            inFlightSeq_ = seq_++;
            retries_ = 0;
            sendInputs(inFlightSeq_);
            return true;
        }
        return false;
    }

    void sendInputs(uint8_t seq) {
        size_t n = 0;
        for (uint8_t i = 0; i < inFlight_; i++) writeInput(scratch_, kMaxPayload, n, inputs_[(inHead_ + i) % kInputQueue]);
        inFlightSentAt_ = now_;
        sendFrame(MSG_INPUT, seq, scratch_, n);
    }

    void sendHello() {
        Hello h;
        h.role = static_cast<uint8_t>(ROLE_PANEL | (transport_ == Transport::Bus ? ROLE_BUS_SLAVE : 0));
        handler_.fillHello(h);
        sendSimple(MSG_HELLO, scratch_, writeHello(scratch_, kMaxPayload, h));
    }

    void sendDescribe() {
        const uint16_t total = handler_.describeCount();
        uint16_t pages = static_cast<uint16_t>((total + kDescribePerPage - 1) / kDescribePerPage);
        if (pages == 0) pages = 1;
        if (pages > 255) pages = 255;
        const uint8_t page = describePage_ < pages ? describePage_ : static_cast<uint8_t>(pages - 1);
        scratch_[0] = page;
        scratch_[1] = static_cast<uint8_t>(pages);
        size_t n = 2;
        for (uint16_t i = static_cast<uint16_t>(page * kDescribePerPage); i < total && n + 3 <= kMaxPayload; i++) {
            const DescribeRecord r = handler_.describeItem(i);
            putU16(scratch_ + n, r.id);
            scratch_[n + 2] = r.role;
            n += 3;
        }
        sendSimple(MSG_DESCRIBE, scratch_, n);
    }

    void sendDiag() {
        Diag d;
        d.crcErrors = decoder_.counters.crcErrors;
        d.framingErrors = decoder_.counters.framingErrors;
        d.overflows = decoder_.counters.overflows;
        d.droppedInputs = droppedInputs_;
        d.lastError = lastError_;
        handler_.fillDiag(d);
        sendSimple(MSG_DIAG, scratch_, writeDiag(scratch_, kMaxPayload, d));
    }

    void sendSyncPage() {
        const uint16_t total = handler_.syncCount();
        size_t n = 1;
        while (syncCursor_ < total && n + 4 <= kMaxPayload) {
            const SyncRecord r = handler_.syncItem(syncCursor_++);
            putU16(scratch_ + n, r.id);
            putU16(scratch_ + n + 2, r.value);
            n += 4;
        }
        const bool last = syncCursor_ >= total;
        scratch_[0] = last ? kSyncLastPage : 0;
        if (last) syncDue_ = false;
        sendSimple(MSG_SYNC_REPORT, scratch_, n);
    }

    void sendSimple(uint8_t type, const uint8_t* p, size_t n) { sendFrame(type, seq_++, p, n); }

    void sendFrame(uint8_t type, uint8_t seq, const uint8_t* p, size_t n) {
        FrameHeader h;
        h.dst = transport_ == Transport::Bus ? ADDR_LINK_NODE : ADDR_BRIDGE;
        h.src = address_;
        h.type = type;
        h.seq = seq;
        h.len = static_cast<uint8_t>(n);
        const size_t w = encodeFrame(h, p, n, wire_, sizeof(wire_));
        if (!w) return;
        port_.beginTransmit();
        port_.write(wire_, w);
        port_.endTransmit();
    }

    Port& port_;
    NodeHandler& handler_;
    FrameDecoder decoder_;
    Transport transport_ = Transport::Usb;
    uint8_t address_ = ADDR_LINK_NODE;
    uint8_t pendingAddress_ = 0;
    uint32_t now_ = 0;
    uint8_t seq_ = 0;

    InputRecord inputs_[kInputQueue];
    uint8_t inHead_ = 0, inCount_ = 0, inFlight_ = 0, inFlightSeq_ = 0, retries_ = 0;
    uint32_t inFlightSentAt_ = 0;
    uint16_t droppedInputs_ = 0;
    uint8_t lastError_ = 0;

    bool helloDue_ = false, subscribeDue_ = false, describeDue_ = false, diagDue_ = false, syncDue_ = false;
    uint8_t describePage_ = 0;
    uint16_t syncCursor_ = 0;

    uint8_t replyType_ = 0, replyLen_ = 0;
    uint8_t reply_[4] = {};

    bool everPolled_ = false, discoverPending_ = false;
    uint32_t lastPolled_ = 0, discoverAt_ = 0;
    uint32_t rng_ = 0x2545F491u;

    uint8_t scratch_[kMaxPayload];
    uint8_t wire_[kMaxWireFrame];
};

} // namespace hn
