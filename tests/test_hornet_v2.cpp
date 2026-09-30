// Tests for the Hornet-native path: protocol v2 framing and records, the
// generated catalogue, input filters, the bridge-side helpers and a simulated
// RS-485 bus with a master and several slaves.

#include "HornetFilters.h"
#include "HornetElements.h"
#include "HornetNative.hpp"
#include "protocol/HnMaster.h"
#include "test_framework.hpp"

#include <algorithm>
#include <cstring>
#include <deque>
#include <random>
#include <string>
#include <vector>

using namespace hornet_native;

/// EXPECT_EQ for strings (the shared macro prints values with unary +).
#define EXPECT_STR_EQ(a, b)                                                        \
    do {                                                                           \
        const std::string sa_ = (a);                                               \
        const std::string sb_ = (b);                                               \
        if (sa_ != sb_) {                                                          \
            std::ostringstream os_;                                                \
            os_ << __FILE__ << ":" << __LINE__ << ": \"" << sa_ << "\" != \"" << sb_ << "\""; \
            throw std::runtime_error(os_.str());                                   \
        }                                                                          \
    } while (0)

namespace {

std::vector<uint8_t> frame(uint8_t dst, uint8_t src, uint8_t type, uint8_t seq, const std::vector<uint8_t>& payload) {
    hn::FrameHeader h;
    h.dst = dst;
    h.src = src;
    h.type = type;
    h.seq = seq;
    std::vector<uint8_t> out(hn::kMaxWireFrame);
    out.resize(hn::encodeFrame(h, payload.data(), payload.size(), out.data(), out.size()));
    return out;
}

/// Feed bytes; return the number of complete valid frames.
int feedAll(hn::FrameDecoder& d, const std::vector<uint8_t>& bytes) {
    int n = 0;
    for (uint8_t b : bytes)
        if (d.feed(b) == hn::FrameDecoder::Result::Frame) n++;
    return n;
}

// ── Simulated links ─────────────────────────────────────────────────────────

struct Medium;

struct SimPort : hn::Port {
    std::deque<uint8_t> rx;
    Medium* medium = nullptr;
    int id = 0;
    bool muted = false; ///< unplugged: sends nothing, hears nothing
    int dropAcks = 0;   ///< lose this many ACK frames addressed to this port
    int read() override {
        if (rx.empty()) return -1;
        const int b = rx.front();
        rx.pop_front();
        return b;
    }
    void write(const uint8_t* d, size_t n) override;
};

/// Half-duplex RS-485 bus. Also checks bus discipline: a slave may only
/// transmit once, right after a POLL addressed to it (or in a DISCOVER window).
struct Medium {
    std::vector<SimPort*> ports; // ports[0] is the master
    hn::FrameDecoder sniff;
    uint8_t lastMasterType = 0;
    uint8_t lastMasterDst = 0;
    int slaveFramesSinceMaster = 0;
    int violations = 0;
    int frames = 0;
    int stateFrames = 0;

    // Every write() carries exactly one frame, so it is sniffed before delivery.
    void transmit(int from, const uint8_t* d, size_t n) {
        uint8_t type = 0, dst = 0;
        for (size_t i = 0; i < n; i++) {
            if (sniff.feed(d[i]) != hn::FrameDecoder::Result::Frame) continue;
            frames++;
            const hn::FrameHeader& h = sniff.header();
            if (h.type == hn::MSG_STATE) stateFrames++;
            type = h.type;
            dst = h.dst;
            if (from == 0) {
                lastMasterType = h.type;
                lastMasterDst = h.dst;
                slaveFramesSinceMaster = 0;
                continue;
            }
            slaveFramesSinceMaster++;
            if (lastMasterType == hn::MSG_DISCOVER) continue;
            if (lastMasterType != hn::MSG_POLL || h.src != lastMasterDst || slaveFramesSinceMaster > 1) violations++;
        }
        for (SimPort* p : ports) {
            if (p->id == from || p->muted) continue;
            if (type == hn::MSG_ACK && dst == p->id && p->dropAcks > 0) { p->dropAcks--; continue; }
            p->rx.insert(p->rx.end(), d, d + n);
        }
    }
};

void SimPort::write(const uint8_t* d, size_t n) {
    if (!muted && medium) medium->transmit(id, d, n);
}

/// Point-to-point USB link (bytes go to a callback).
struct UsbPort : hn::Port {
    std::deque<uint8_t> rx;
    std::function<void(const uint8_t*, size_t)> out;
    int read() override {
        if (rx.empty()) return -1;
        const int b = rx.front();
        rx.pop_front();
        return b;
    }
    void write(const uint8_t* d, size_t n) override { if (out) out(d, n); }
};

struct TestPanel : hn::NodeHandler {
    std::string name = "TEST";
    std::vector<hn::SyncRecord> positions;
    std::vector<uint16_t> outputs;
    std::vector<hn::StateRecord> received;
    std::vector<std::string> texts;
    uint8_t boardId[8] = {};
    uint8_t mode = hn::MODE_SIM;

    void fillHello(hn::Hello& h) override {
        h.catalogHash = Hornet::kCatalogHash;
        std::strncpy(h.name, name.c_str(), hn::kMaxName);
        std::memcpy(h.boardId, boardId, sizeof(boardId));
    }
    void onState(const hn::StateRecord& r) override {
        for (uint16_t id : outputs)
            if (id == r.id) {
                received.push_back(r);
                if (r.kind == hn::VALUE_TEXT) texts.emplace_back(reinterpret_cast<const char*>(r.text), r.textLen);
            }
    }
    uint8_t onMode(uint8_t m) override { mode = m; return m; }
    uint16_t syncCount() override { return static_cast<uint16_t>(positions.size()); }
    hn::SyncRecord syncItem(uint16_t i) override { return positions[i]; }
    uint16_t describeCount() override { return static_cast<uint16_t>(positions.size() + outputs.size()); }
    hn::DescribeRecord describeItem(uint16_t i) override {
        hn::DescribeRecord r;
        if (i < positions.size()) { r.id = positions[i].id; r.role = hn::DESCRIBE_ROLE_INPUT; }
        else { r.id = outputs[i - positions.size()]; r.role = hn::DESCRIBE_ROLE_OUTPUT; }
        return r;
    }
    size_t writeSubscribe(uint8_t* buf, size_t cap) override {
        buf[0] = hn::SUBSCRIBE_CONTROLS;
        size_t n = 1;
        for (uint16_t id : outputs) { if (n + 2 > cap) break; hn::putU16(buf + n, id); n += 2; }
        return n;
    }
};

hn::InputRecord input(uint16_t id, uint8_t action, uint16_t arg) {
    hn::InputRecord r;
    r.id = id;
    r.action = action;
    r.arg = arg;
    return r;
}

const uint16_t kMasterArm = Hornet::MasterArm::MasterArm.id;
const uint16_t kKey1 = Hornet::UFC::Key1.id;
const uint16_t kAaLt = Hornet::MasterArm::AaLt.id;
const uint16_t kScratch = Hornet::UFC::ScratchpadNumber.id;
const uint16_t kComm1Vol = Hornet::UFC::Comm1Vol.id;

std::vector<std::string> displayedText;

void captureText(const char* text, uint8_t length) {
    displayedText.emplace_back(text, length);
}

// ── Test groups ─────────────────────────────────────────────────────────────

void framingTests() {
    auto s = createSuite("Protocol v2 - CRC, COBS, frames");

    addTest(s, "CRC-16/CCITT-FALSE check value", []() {
        const char* v = "123456789";
        EXPECT_EQ(hn::crc16(reinterpret_cast<const uint8_t*>(v), 9), 0x29B1);
    });

    addTest(s, "COBS round-trips zeros, long runs and random data", []() {
        std::mt19937 rng(42);
        const size_t sizes[] = {0, 1, 2, 253, 254, 255, 256, 300, 508};
        for (size_t size : sizes) {
            for (int pattern = 0; pattern < 3; pattern++) {
                std::vector<uint8_t> raw(size);
                for (size_t i = 0; i < size; i++)
                    raw[i] = pattern == 0 ? 0 : pattern == 1 ? 0xAB : static_cast<uint8_t>(rng() % 4 == 0 ? 0 : rng());
                std::vector<uint8_t> enc(size + size / 254 + 3);
                hn::CobsWriter w(enc.data(), enc.size());
                for (uint8_t b : raw) w.put(b);
                const size_t n = w.finish();
                EXPECT_TRUE(n > 0);
                EXPECT_EQ(enc[n - 1], 0);
                for (size_t i = 0; i + 1 < n; i++) EXPECT_TRUE(enc[i] != 0);
                const int dec = hn::cobsDecodeInPlace(enc.data(), n - 1);
                EXPECT_EQ(dec, static_cast<int>(size));
                EXPECT_TRUE(std::equal(raw.begin(), raw.end(), enc.begin()));
            }
        }
    });

    addTest(s, "COBS writer reports a too-small buffer", []() {
        uint8_t out[4];
        hn::CobsWriter w(out, sizeof(out));
        for (int i = 0; i < 10; i++) w.put(1);
        EXPECT_EQ(w.finish(), static_cast<size_t>(0));
        EXPECT_TRUE(!w.ok());
    });

    addTest(s, "Frame encode/decode keeps header and payload", []() {
        std::vector<uint8_t> p = {0, 1, 2, 0, 0, 0xFE, 0xFF};
        auto f = frame(5, hn::ADDR_BRIDGE, hn::MSG_STATE, 77, p);
        hn::FrameDecoder d;
        EXPECT_EQ(feedAll(d, f), 1);
        EXPECT_EQ(d.header().dst, 5);
        EXPECT_EQ(d.header().src, hn::ADDR_BRIDGE);
        EXPECT_EQ(d.header().type, hn::MSG_STATE);
        EXPECT_EQ(d.header().seq, 77);
        EXPECT_EQ(d.header().len, 7);
        EXPECT_TRUE(std::memcmp(d.payload(), p.data(), p.size()) == 0);
    });

    addTest(s, "Maximum payload fits; larger is refused", []() {
        std::vector<uint8_t> p(hn::kMaxPayload, 0);
        auto f = frame(1, 2, hn::MSG_STATE, 0, p);
        EXPECT_TRUE(!f.empty() && f.size() <= hn::kMaxWireFrame);
        hn::FrameDecoder d;
        EXPECT_EQ(feedAll(d, f), 1);
        std::vector<uint8_t> big(hn::kMaxPayload + 1, 1);
        hn::FrameHeader h;
        uint8_t out[400];
        EXPECT_EQ(hn::encodeFrame(h, big.data(), big.size(), out, sizeof(out)), static_cast<size_t>(0));
    });

    addTest(s, "Corrupted byte is a CRC error, next frame still decodes", []() {
        auto a = frame(1, 2, hn::MSG_INPUT, 1, {1, 2, 3, 4, 5});
        auto b = frame(1, 2, hn::MSG_INPUT, 2, {9, 9, 9, 9, 9});
        a[3] ^= 0x10;
        if (a[3] == 0) a[3] = 0x33;
        hn::FrameDecoder d;
        std::vector<uint8_t> all(a);
        all.insert(all.end(), b.begin(), b.end());
        EXPECT_EQ(feedAll(d, all), 1);
        EXPECT_EQ(d.header().seq, 2);
        EXPECT_EQ(d.counters.crcErrors + d.counters.framingErrors, 1);
    });

    addTest(s, "Garbage and oversized input are dropped until the next delimiter", []() {
        hn::FrameDecoder d;
        std::vector<uint8_t> junk(500, 0x55);
        junk.push_back(0);
        auto good = frame(3, 4, hn::MSG_HEARTBEAT, 9, {});
        junk.insert(junk.end(), good.begin(), good.end());
        EXPECT_EQ(feedAll(d, junk), 1);
        EXPECT_EQ(d.counters.overflows, 1);
        EXPECT_EQ(d.header().seq, 9);
    });

    addTest(s, "Wrong version and length mismatch are rejected", []() {
        hn::FrameHeader h;
        h.version = 1;
        uint8_t out[32];
        size_t n = hn::encodeFrame(h, nullptr, 0, out, sizeof(out));
        hn::FrameDecoder d;
        for (size_t i = 0; i < n; i++) d.feed(out[i]);
        EXPECT_EQ(d.counters.versionErrors, 1);
        // Hand-build a frame whose len field lies.
        uint8_t raw[] = {2, 1, 2, hn::MSG_STATE, 0, 5, 0xAA, 0, 0};
        const uint16_t crc = hn::crc16(raw, 7);
        raw[7] = static_cast<uint8_t>(crc);
        raw[8] = static_cast<uint8_t>(crc >> 8);
        uint8_t enc[16];
        hn::CobsWriter w(enc, sizeof(enc));
        for (uint8_t b : raw) w.put(b);
        const size_t m = w.finish();
        for (size_t i = 0; i < m; i++) d.feed(enc[i]);
        EXPECT_EQ(d.counters.framingErrors, 1);
        EXPECT_EQ(d.counters.frames, 0);
    });

    addTest(s, "Duplicate filter accepts a sequence number once per source", []() {
        hn::DuplicateFilter f;
        EXPECT_TRUE(f.accept(1, 5));
        EXPECT_TRUE(!f.accept(1, 5));
        EXPECT_TRUE(f.accept(2, 5));
        EXPECT_TRUE(f.accept(1, 6));
        f.forget(1);
        EXPECT_TRUE(f.accept(1, 6));
    });
}

void recordTests() {
    auto s = createSuite("Protocol v2 - typed records");

    addTest(s, "STATE writer/reader for every value kind", []() {
        uint8_t buf[hn::kMaxPayload];
        hn::StateWriter w(buf, sizeof(buf));
        const uint8_t txt[] = {'1', '2', '3', '4'};
        EXPECT_TRUE(w.addBool(kAaLt, true));
        EXPECT_TRUE(w.addPosition(kMasterArm, 1));
        EXPECT_TRUE(w.addAnalog(kComm1Vol, 40000));
        EXPECT_TRUE(w.addText(kScratch, txt, 4));
        hn::StateReader r(buf, w.size());
        hn::StateRecord rec;
        EXPECT_TRUE(r.next(rec) && rec.id == kAaLt && rec.kind == hn::VALUE_BOOL && rec.value == 1);
        EXPECT_TRUE(r.next(rec) && rec.id == kMasterArm && rec.value == 1);
        EXPECT_TRUE(r.next(rec) && rec.kind == hn::VALUE_ANALOG && rec.value == 40000);
        EXPECT_TRUE(r.next(rec) && rec.kind == hn::VALUE_TEXT && rec.textLen == 4 && std::memcmp(rec.text, "1234", 4) == 0);
        EXPECT_TRUE(!r.next(rec));
        EXPECT_TRUE(!r.error());
    });

    addTest(s, "STATE writer refuses records that do not fit", []() {
        uint8_t buf[hn::kMaxPayload];
        hn::StateWriter w(buf, sizeof(buf));
        int n = 0;
        while (w.addAnalog(static_cast<uint16_t>(n), 1)) n++;
        EXPECT_EQ(n, static_cast<int>(hn::kMaxPayload / 5));
        EXPECT_TRUE(w.size() <= hn::kMaxPayload);
    });

    addTest(s, "Truncated or unknown STATE records set error()", []() {
        const uint8_t truncText[] = {0x40, 0x02, hn::VALUE_TEXT, 8, 'a', 'b'};
        hn::StateReader r1(truncText, sizeof(truncText));
        hn::StateRecord rec;
        EXPECT_TRUE(!r1.next(rec) && r1.error());
        const uint8_t badKind[] = {1, 1, 9, 0};
        hn::StateReader r2(badKind, sizeof(badKind));
        EXPECT_TRUE(!r2.next(rec) && r2.error());
    });

    addTest(s, "INPUT records round-trip; odd lengths are errors", []() {
        uint8_t buf[hn::kMaxPayload];
        size_t n = 0;
        EXPECT_TRUE(hn::writeInput(buf, sizeof(buf), n, input(kKey1, hn::ACTION_PRESS, 1)));
        EXPECT_TRUE(hn::writeInput(buf, sizeof(buf), n, input(0x0221, hn::ACTION_STEP, static_cast<uint16_t>(-3))));
        hn::InputReader r(buf, n);
        hn::InputRecord rec;
        EXPECT_TRUE(r.next(rec) && rec.id == kKey1 && rec.action == hn::ACTION_PRESS);
        EXPECT_TRUE(r.next(rec) && rec.step() == -3);
        EXPECT_TRUE(!r.next(rec) && !r.error());
        hn::InputReader bad(buf, n - 1);
        EXPECT_TRUE(!bad.next(rec) && bad.error());
    });

    addTest(s, "HELLO and DIAG round-trip and sanitise text", []() {
        hn::Hello h;
        h.role = hn::ROLE_PANEL | hn::ROLE_BUS_SLAVE;
        h.fwMajor = 2;
        h.catalogHash = 0x12345678;
        for (int i = 0; i < 8; i++) h.boardId[i] = static_cast<uint8_t>(i * 3);
        std::strcpy(h.name, "UFC");
        uint8_t buf[hn::kMaxPayload];
        const size_t n = hn::writeHello(buf, sizeof(buf), h);
        buf[17] = 0x01; // control char in the name
        hn::Hello out;
        EXPECT_TRUE(hn::readHello(buf, n, out));
        EXPECT_EQ(out.catalogHash, 0x12345678u);
        EXPECT_STR_EQ(std::string(out.name), std::string("?FC"));
        EXPECT_TRUE(!hn::readHello(buf, n - 1, out));

        hn::Diag d;
        d.crcErrors = 3;
        d.freeRam = 900;
        std::strcpy(d.text, "ok");
        const size_t m = hn::writeDiag(buf, sizeof(buf), d);
        hn::Diag e;
        EXPECT_TRUE(hn::readDiag(buf, m, e));
        EXPECT_EQ(e.crcErrors, 3);
        EXPECT_EQ(e.freeRam, 900);
        EXPECT_STR_EQ(std::string(e.text), std::string("ok"));
    });
}

void catalogTests() {
    auto s = createSuite("Catalogue - generated F/A-18C table");

    addTest(s, "Table is sorted, unique and matches the count", []() {
        const Hornet::ControlInfo* t = Hornet::catalogTable();
        for (uint16_t i = 1; i < Hornet::kCatalogControlCount; i++) EXPECT_TRUE(t[i - 1].id < t[i].id);
        EXPECT_TRUE(Hornet::kCatalogControlCount > 100);
    });

    addTest(s, "Every positional control names each position", []() {
        const Hornet::ControlInfo* t = Hornet::catalogTable();
        for (uint16_t i = 0; i < Hornet::kCatalogControlCount; i++) {
            const auto k = static_cast<Hornet::Kind>(t[i].kind);
            if (k != Hornet::Kind::Switch && k != Hornet::Kind::Selector && k != Hornet::Kind::Button) continue;
            EXPECT_TRUE(t[i].positions != nullptr);
            int fields = 1;
            for (const char* p = t[i].positions; *p; p++) fields += *p == '|';
            EXPECT_EQ(fields, static_cast<int>(t[i].count));
        }
    });

    addTest(s, "Name and position lookups", []() {
        Hornet::ControlInfo row{};
        EXPECT_TRUE(Hornet::findControl(kMasterArm, row));
        char buf[40];
        Hornet::copyFlashString(row.name, buf, sizeof(buf));
        EXPECT_STR_EQ(std::string(buf), std::string("MASTER_ARM.MASTER_ARM"));
        Hornet::positionName(row, Hornet::MasterArm::MasterArm.ARM, buf, sizeof(buf));
        EXPECT_STR_EQ(std::string(buf), std::string("ARM"));
        EXPECT_EQ(Hornet::positionIndex(row, "SAFE"), 0);
        EXPECT_EQ(Hornet::positionIndex(row, "ARM"), 1);
        EXPECT_EQ(Hornet::positionIndex(row, "AR"), -1);
        Hornet::ControlInfo byName{};
        EXPECT_TRUE(Hornet::findControlByName("UFC.KEY_1", byName));
        EXPECT_EQ(byName.id, kKey1);
        EXPECT_TRUE(!Hornet::findControlByName("UFC.KEY_", byName));
        EXPECT_STR_EQ(controlName(0xEE01), std::string("0xEE01"));
        EXPECT_TRUE(!Hornet::findControl(0x0000, row));
    });

    addTest(s, "Typed controls carry kind and position count", []() {
        static_assert(decltype(Hornet::MasterArm::MasterArm)::kind() == Hornet::Kind::Switch, "kind");
        static_assert(decltype(Hornet::MasterArm::MasterArm)::count() == 2, "count");
        static_assert(decltype(Hornet::UFC::ScratchpadNumber)::kind() == Hornet::Kind::Text, "text");
        EXPECT_EQ(Hornet::MasterArm::MasterArm.panel(), Hornet::MasterArm::kPanelId);
    });
}

void filterTests() {
    auto s = createSuite("Library - debounce, pots, encoders, curves");

    addTest(s, "Debouncer needs a stable reading", []() {
        Hornet::Debouncer d(10);
        d.reset(0, 0);
        EXPECT_TRUE(!d.update(1, 1));
        EXPECT_TRUE(!d.update(0, 3));   // bounce
        EXPECT_TRUE(!d.update(1, 5));
        EXPECT_TRUE(!d.update(1, 12));
        EXPECT_TRUE(d.update(1, 16));
        EXPECT_EQ(d.position(), 1);
    });

    addTest(s, "Pot filter: deadband, rate limit, settle", []() {
        Hornet::PotFilter f;
        f.smoothing = 0;
        uint16_t out = 0;
        EXPECT_TRUE(f.update(1000, 0, out) && out == 1000);
        EXPECT_TRUE(!f.update(1100, 30, out));                 // inside deadband
        EXPECT_TRUE(f.update(5000, 60, out) && out == 5000);   // big move
        EXPECT_TRUE(!f.update(9000, 65, out));                 // rate limited
        EXPECT_TRUE(f.update(9000, 85, out) && out == 9000);
        EXPECT_TRUE(!f.update(9100, 100, out));
        EXPECT_TRUE(f.update(9100, 240, out) && out == 9100);  // settled: exact value
        EXPECT_TRUE(!f.update(9100, 500, out));
    });

    addTest(s, "Pot smoothing reduces noise", []() {
        Hornet::PotFilter f;
        f.smoothing = 4;
        f.deadband = 1;
        f.minIntervalMs = 0;
        uint16_t out = 0;
        f.update(30000, 0, out);
        int reports = 0;
        for (int i = 1; i < 100; i++) reports += f.update(static_cast<uint16_t>(30000 + ((i & 1) ? 300 : -300)), static_cast<uint32_t>(i), out);
        EXPECT_TRUE(out > 29800 && out < 30200);
        EXPECT_TRUE(reports > 0);
    });

    addTest(s, "Quadrature decoder: one step per detent, bounce ignored", []() {
        Hornet::QuadratureDecoder q;
        q.reset(false, false);
        // Clockwise Gray sequence 00 -> 10 -> 11 -> 01 -> 00 (a is bit 1).
        const bool cw[4][2] = {{true, false}, {true, true}, {false, true}, {false, false}};
        int total = 0;
        for (auto& st : cw) total += q.update(st[0], st[1], 0);
        EXPECT_EQ(total, 1);
        for (int i = 3; i >= 0; i--) total += q.update(i ? cw[i - 1][0] : false, i ? cw[i - 1][1] : false, 0);
        EXPECT_EQ(total, 0);
        total = 0;
        for (int i = 0; i < 10; i++) total += q.update(true, false, 0) + q.update(false, false, 0);
        EXPECT_EQ(total, 0);
    });

    addTest(s, "Encoder acceleration multiplies fast detents", []() {
        Hornet::QuadratureDecoder q;
        q.acceleration = true;
        q.reset(false, false);
        const bool cw[4][2] = {{true, false}, {true, true}, {false, true}, {false, false}};
        int total = 0;
        for (int d = 0; d < 4; d++)
            for (auto& st : cw) total += q.update(st[0], st[1], static_cast<uint32_t>(d * 10));
        EXPECT_TRUE(total > 4);
    });

    addTest(s, "Calibration curve and axis scaling", []() {
        const Hornet::CurvePoint c[] = {{0, 1000}, {32768, 1500}, {65535, 2000}};
        EXPECT_EQ(Hornet::applyCurve(c, 3, 0), 1000);
        EXPECT_EQ(Hornet::applyCurve(c, 3, 16384), 1250);
        EXPECT_EQ(Hornet::applyCurve(c, 3, 65535), 2000);
        EXPECT_EQ(Hornet::scaleToAxis(100, 1000, 60000), 0);
        EXPECT_EQ(Hornet::scaleToAxis(60000, 1000, 60000), 65535);
    });
}

void elementTests() {
    auto s = createSuite("Library - text display lamp test");

    addTest(s, "Text display restores the latest cached value after lamp test", []() {
        displayedText.clear();
        Hornet::TextDisplay display(Hornet::UFC::ScratchpadNumber, captureText);
        const uint8_t initial[] = {'1', '2', '3', '4'};
        hn::StateRecord state;
        state.id = kScratch;
        state.kind = hn::VALUE_TEXT;
        state.text = initial;
        state.textLen = sizeof(initial);
        display.onState(state);
        EXPECT_STR_EQ(displayedText.back(), std::string("1234    "));

        display.onMode(hn::MODE_LAMP_TEST);
        EXPECT_STR_EQ(displayedText.back(), std::string("88888888"));

        const uint8_t latest[] = {'A', 'B'};
        state.text = latest;
        state.textLen = sizeof(latest);
        display.onState(state);
        EXPECT_EQ(displayedText.size(), static_cast<size_t>(2));

        display.onMode(hn::MODE_SIM);
        EXPECT_STR_EQ(displayedText.back(), std::string("AB      "));
    });
}

void nativeTests() {
    auto s = createSuite("Bridge - native exporter, inputs, sync");

    addTest(s, "Exporter datagram updates state by id", []() {
        CatalogState st;
        DatagramHeader h;
        ParseStats ps;
        const std::string dg = "HLN 2 0x" + std::string([] { char b[9]; std::snprintf(b, sizeof(b), "%08X", static_cast<unsigned>(Hornet::kCatalogHash)); return std::string(b); }()) +
                               " FA-18C_hornet 1 1\n0101=1\n0104=1\n0240=T:%2012%3D4\nEE01=1\n021E=40000\nzz\n0101=7\n";
        EXPECT_TRUE(parseExporterDatagram(dg, st, h, ps) == ParseResult::Ok);
        EXPECT_EQ(h.catalogHash, Hornet::kCatalogHash);
        EXPECT_TRUE(h.active && h.full);
        EXPECT_STR_EQ(h.aircraft, std::string("FA-18C_hornet"));
        EXPECT_EQ(st.get(kMasterArm)->number, 1);
        EXPECT_EQ(st.get(kAaLt)->kind, hn::VALUE_BOOL);
        EXPECT_STR_EQ(st.get(kScratch)->text, std::string("    12=4").substr(0, 8));
        EXPECT_EQ(st.get(kComm1Vol)->number, 40000);
        EXPECT_EQ(ps.unknownIds, 1u);
        EXPECT_EQ(ps.badLines, 2u); // "zz" and the out-of-range position
        EXPECT_EQ(st.takeDirty().size(), static_cast<size_t>(4));
        EXPECT_TRUE(parseExporterDatagram(dg, st, h, ps) == ParseResult::Ok);
        EXPECT_TRUE(st.takeDirty().empty());
    });

    addTest(s, "Bad or foreign datagrams are refused", []() {
        CatalogState st;
        DatagramHeader h;
        ParseStats ps;
        EXPECT_TRUE(parseExporterDatagram("DCS-BIOS", st, h, ps) == ParseResult::NotHornetLink);
        EXPECT_TRUE(parseExporterDatagram("HLN 2 zz F18 1 1", st, h, ps) == ParseResult::BadHeader);
        EXPECT_TRUE(parseExporterDatagram("HLN 2 0x1 F18 2 1", st, h, ps) == ParseResult::BadHeader);
        EXPECT_TRUE(parseExporterDatagram("HLN 2 0x1 F18 1", st, h, ps) == ParseResult::BadHeader);
        EXPECT_TRUE(parseExporterDatagram("HLN 2 0x1 F18 0 1", st, h, ps) == ParseResult::Ok);
        EXPECT_TRUE(!h.active);
    });

    addTest(s, "Inputs are validated against the catalogue", []() {
        EXPECT_EQ(validateInput(input(kMasterArm, hn::ACTION_SET_POSITION, 1)), 0);
        EXPECT_EQ(validateInput(input(kMasterArm, hn::ACTION_SET_POSITION, 2)), hn::NACK_BAD_VALUE);
        EXPECT_EQ(validateInput(input(kMasterArm, hn::ACTION_ANALOG, 2)), hn::NACK_BAD_VALUE);
        EXPECT_EQ(validateInput(input(kKey1, hn::ACTION_PRESS, 1)), 0);
        EXPECT_EQ(validateInput(input(kAaLt, hn::ACTION_SET_POSITION, 1)), hn::NACK_BAD_VALUE);
        EXPECT_EQ(validateInput(input(0xEE01, hn::ACTION_PRESS, 1)), hn::NACK_UNKNOWN_CONTROL);
        EXPECT_EQ(validateInput(input(kComm1Vol, hn::ACTION_ANALOG, 1234)), 0);
    });

    addTest(s, "Exporter lines and readable descriptions", []() {
        EXPECT_STR_EQ(formatExporterInput(input(kMasterArm, hn::ACTION_SET_POSITION, 1)), std::string("0101 SET 1"));
        EXPECT_STR_EQ(formatExporterInput(input(0x0221, hn::ACTION_STEP, static_cast<uint16_t>(-2))), std::string("0221 STEP -2"));
        EXPECT_STR_EQ(formatExporterInput(input(kKey1, hn::ACTION_PRESS, 1)), std::string("0202 PRESS"));
        EXPECT_STR_EQ(describeInput(input(kMasterArm, hn::ACTION_SET_POSITION, 1)), std::string("MASTER_ARM.MASTER_ARM=ARM"));
        EXPECT_STR_EQ(describeInput(input(kKey1, hn::ACTION_PRESS, 1)), std::string("UFC.KEY_1 PRESS"));
    });

    addTest(s, "Sync tracker lists cockpit/DCS differences (DCS wins)", []() {
        CatalogState sim;
        Value v;
        v.number = 0;
        sim.set(kMasterArm, v);
        Value pot;
        pot.kind = hn::VALUE_ANALOG;
        pot.number = 30000;
        sim.set(kComm1Vol, pot);
        SyncTracker t;
        t.setCockpit(kMasterArm, 1);
        t.setCockpit(kComm1Vol, 31000);
        t.setCockpit(kKey1, 1); // buttons are ignored
        auto d = t.discrepancies(sim);
        EXPECT_EQ(d.size(), static_cast<size_t>(1));
        EXPECT_STR_EQ(d[0].name, std::string("MASTER_ARM.MASTER_ARM"));
        EXPECT_STR_EQ(d[0].cockpit, std::string("ARM"));
        EXPECT_STR_EQ(d[0].sim, std::string("SAFE"));
        EXPECT_TRUE(t.overlayText(sim).find("MASTER_ARM.MASTER_ARM: set SAFE (is ARM)") != std::string::npos);
        t.overlayEnabled = false;
        EXPECT_TRUE(t.overlayText(sim).empty());
        t.setCockpit(kMasterArm, 0);
        EXPECT_TRUE(t.discrepancies(sim).empty());
    });

    addTest(s, "STATE frames split at the payload limit and decode back", []() {
        CatalogState st;
        std::vector<uint16_t> ids;
        const Hornet::ControlInfo* t = Hornet::catalogTable();
        for (uint16_t i = 0; i < Hornet::kCatalogControlCount; i++) {
            Value v;
            v.kind = valueKindFor(t[i].kind);
            if (v.kind == hn::VALUE_TEXT) v.text.assign(t[i].count, 'X');
            st.set(t[i].id, v);
            ids.push_back(t[i].id);
        }
        uint8_t seq = 0;
        auto frames = buildStateFrames(st, ids, hn::ADDR_BROADCAST, seq);
        EXPECT_TRUE(frames.size() > 1);
        hn::FrameDecoder d;
        size_t records = 0;
        for (auto& f : frames) {
            EXPECT_EQ(feedAll(d, f), 1);
            hn::StateReader r(d.payload(), d.header().len);
            hn::StateRecord rec;
            while (r.next(rec)) records++;
            EXPECT_TRUE(!r.error());
        }
        EXPECT_EQ(records, ids.size());
    });

    addTest(s, "Frame decoder prints names", []() {
        uint8_t p[5];
        size_t n = 0;
        hn::writeInput(p, sizeof(p), n, input(kKey1, hn::ACTION_PRESS, 1));
        hn::FrameHeader h;
        h.src = 3;
        h.dst = hn::ADDR_LINK_NODE;
        h.type = hn::MSG_INPUT;
        h.seq = 7;
        EXPECT_STR_EQ(decodeFrame(h, p, n), std::string("BUS3 -> LINK INPUT #7: UFC.KEY_1 PRESS"));
    });
}

// ── USB panel against the bridge session ───────────────────────────────────

struct UsbRig {
    UsbPort port;
    TestPanel panel;
    hn::Node node{port, panel};
    LinkSession bridge;
    SyncTracker sync;
    std::vector<hn::InputRecord> forwarded;
    bool dropBridgeToNode = false;
    uint32_t now = 0;

    UsbRig() {
        port.out = [this](const uint8_t* d, size_t n) { bridge.feed(d, n); };
        bridge.write = [this](const uint8_t* d, size_t n) { if (!dropBridgeToNode) port.rx.insert(port.rx.end(), d, d + n); };
        bridge.onInput = [this](uint8_t, const hn::InputRecord& r) { forwarded.push_back(r); };
        bridge.sync = &sync;
        panel.positions = {{kMasterArm, 1}};
        panel.outputs = {kAaLt, kScratch};
    }
    void run(uint32_t ms) { for (uint32_t i = 0; i < ms; i++) node.update(++now); }
};

void usbTests() {
    auto s = createSuite("Protocol v2 - USB panel <-> bridge");

    addTest(s, "Connect: HELLO, SUBSCRIBE and SYNC_REPORT reach the bridge", []() {
        UsbRig rig;
        rig.node.beginUsb();
        rig.run(5);
        EXPECT_TRUE(rig.bridge.v2Confirmed());
        const auto& nodes = rig.bridge.nodes();
        EXPECT_TRUE(nodes.count(hn::ADDR_LINK_NODE) == 1);
        const NodeInfo& n = nodes.at(hn::ADDR_LINK_NODE);
        EXPECT_TRUE(n.haveHello && !n.catalogMismatch);
        EXPECT_TRUE(n.subscription.wants(kAaLt) && !n.subscription.wants(kKey1));
        CatalogState sim;
        Value v;
        sim.set(kMasterArm, v);
        EXPECT_EQ(rig.sync.discrepancies(sim).size(), static_cast<size_t>(1));
    });

    addTest(s, "Inputs are acknowledged and forwarded once", []() {
        UsbRig rig;
        rig.node.beginUsb();
        rig.run(5);
        rig.node.queueInput(input(kKey1, hn::ACTION_PRESS, 1));
        rig.node.queueInput(input(kKey1, hn::ACTION_RELEASE, 0));
        rig.run(3);
        EXPECT_EQ(rig.forwarded.size(), static_cast<size_t>(2));
        EXPECT_TRUE(!rig.node.inputsPending());
    });

    addTest(s, "Lost ACK: retried with the same sequence, forwarded once", []() {
        UsbRig rig;
        rig.node.beginUsb();
        rig.run(5);
        rig.dropBridgeToNode = true;
        rig.node.queueInput(input(kMasterArm, hn::ACTION_SET_POSITION, 1));
        rig.run(150); // one retry after 100 ms
        rig.dropBridgeToNode = false;
        rig.run(150);
        EXPECT_EQ(rig.forwarded.size(), static_cast<size_t>(1));
        EXPECT_TRUE(!rig.node.inputsPending());
        EXPECT_EQ(rig.node.droppedInputs(), 0);
    });

    addTest(s, "No bridge: input dropped after the retry budget", []() {
        UsbRig rig;
        rig.node.beginUsb();
        rig.run(5);
        rig.dropBridgeToNode = true;
        rig.node.queueInput(input(kMasterArm, hn::ACTION_SET_POSITION, 1));
        rig.run(600);
        EXPECT_TRUE(!rig.node.inputsPending());
        EXPECT_EQ(rig.node.droppedInputs(), 1);
    });

    addTest(s, "Pot updates waiting in the queue are merged", []() {
        UsbPort port;
        TestPanel panel;
        hn::Node node(port, panel);
        for (uint16_t v = 0; v < 100; v++) node.queueInput(input(kComm1Vol, hn::ACTION_ANALOG, v));
        EXPECT_EQ(node.droppedInputs(), 0);
    });

    addTest(s, "Bad input gets a NACK with a reason; wrong mode is refused", []() {
        UsbRig rig;
        rig.node.beginUsb();
        rig.run(5);
        rig.node.queueInput(input(kAaLt, hn::ACTION_SET_POSITION, 1)); // a lamp is not an input
        rig.run(3);
        EXPECT_EQ(rig.node.lastError(), hn::NACK_BAD_VALUE);
        EXPECT_TRUE(!rig.node.inputsPending());
        rig.bridge.setMode(hn::MODE_LAMP_TEST);
        rig.run(3);
        EXPECT_EQ(rig.panel.mode, hn::MODE_LAMP_TEST);
        rig.node.queueInput(input(kKey1, hn::ACTION_PRESS, 1));
        rig.run(3);
        EXPECT_EQ(rig.node.lastError(), hn::NACK_WRONG_MODE);
        EXPECT_TRUE(rig.forwarded.empty());
    });

    addTest(s, "STATE broadcast reaches subscribed outputs", []() {
        UsbRig rig;
        rig.node.beginUsb();
        rig.run(5);
        CatalogState st;
        Value lamp;
        lamp.kind = hn::VALUE_BOOL;
        lamp.number = 1;
        st.set(kAaLt, lamp);
        Value text;
        text.kind = hn::VALUE_TEXT;
        text.text = "  1234  ";
        st.set(kScratch, text);
        Value other;
        st.set(kMasterArm, other);
        rig.bridge.sendState(st, st.allIds());
        rig.run(2);
        EXPECT_EQ(rig.panel.received.size(), static_cast<size_t>(2));
        EXPECT_STR_EQ(rig.panel.texts.at(0), std::string("  1234  "));
    });

    addTest(s, "A USB burst of MODE requests receives one reply per request", []() {
        UsbPort port;
        TestPanel panel;
        hn::Node node(port, panel);
        hn::FrameDecoder decoder;
        std::vector<uint8_t> responseTypes;
        port.out = [&](const uint8_t* bytes, size_t len) {
            for (size_t i = 0; i < len; i++)
                if (decoder.feed(bytes[i]) == hn::FrameDecoder::Result::Frame)
                    responseTypes.push_back(decoder.header().type);
        };
        node.beginUsb();
        node.update(0);
        responseTypes.clear();

        for (uint8_t i = 0; i < 20; i++) {
            const uint8_t mode = i & 1 ? hn::MODE_LAMP_TEST : hn::MODE_SIM;
            const auto request = frame(hn::ADDR_LINK_NODE, hn::ADDR_BRIDGE, hn::MSG_MODE, i, {mode});
            port.rx.insert(port.rx.end(), request.begin(), request.end());
        }
        node.update(1);

        EXPECT_EQ(std::count(responseTypes.begin(), responseTypes.end(), hn::MSG_MODE_ACK), 20);
        EXPECT_EQ(panel.mode, hn::MODE_LAMP_TEST);
    });
}

// ── RS-485 bus simulation ──────────────────────────────────────────────────

struct BusRig {
    Medium medium;
    SimPort masterBus;
    UsbPort masterUsb;
    hn::BusMaster master{masterUsb, masterBus};
    SimPort slavePorts[3];
    TestPanel panels[3];
    std::unique_ptr<hn::Node> nodes[3];
    LinkSession bridge;
    std::vector<std::pair<uint8_t, hn::InputRecord>> forwarded;
    std::vector<uint32_t> forwardTimes;
    uint32_t now = 0;
    bool frozen[3] = {false, false, false};

    BusRig() {
        masterBus.id = 0;
        masterBus.medium = &medium;
        medium.ports.push_back(&masterBus);
        for (int i = 0; i < 3; i++) {
            slavePorts[i].id = i + 1;
            slavePorts[i].medium = &medium;
            medium.ports.push_back(&slavePorts[i]);
            panels[i].name = "SLAVE" + std::to_string(i + 1);
            panels[i].outputs = {static_cast<uint16_t>(kAaLt + i)};
            panels[i].positions = {{kMasterArm, 0}};
            nodes[i].reset(new hn::Node(slavePorts[i], panels[i]));
            nodes[i]->beginBus(static_cast<uint8_t>(i + 1));
            master.addSlave(static_cast<uint8_t>(i + 1));
        }
        masterUsb.out = [this](const uint8_t* d, size_t n) { bridge.feed(d, n); };
        bridge.write = [this](const uint8_t* d, size_t n) { masterUsb.rx.insert(masterUsb.rx.end(), d, d + n); };
        bridge.onInput = [this](uint8_t src, const hn::InputRecord& r) { forwarded.push_back({src, r}); forwardTimes.push_back(now); };
        master.setIdentity("MASTER", 2, 0, nullptr);
    }
    void run(uint32_t ms) {
        for (uint32_t t = 0; t < ms; t++) {
            now++;
            master.update(now);
            for (int i = 0; i < 3; i++) if (!frozen[i]) nodes[i]->update(now);
        }
    }
};

void busTests() {
    auto s = createSuite("Protocol v2 - simulated RS-485 bus (master + 3 slaves)");

    addTest(s, "All slaves join and identify; no slave ever talks out of turn", []() {
        BusRig rig;
        rig.bridge.start();
        rig.run(1500);
        EXPECT_EQ(rig.medium.violations, 0);
        for (uint8_t a = 1; a <= 3; a++) {
            EXPECT_TRUE(rig.bridge.nodes().count(a) == 1);
            EXPECT_TRUE(rig.bridge.nodes().at(a).haveHello);
            EXPECT_TRUE(rig.bridge.nodes().at(a).online);
        }
        for (uint8_t i = 0; i < rig.master.slaveCount(); i++) EXPECT_TRUE(rig.master.slave(i).online);
    });

    addTest(s, "Inputs from every slave arrive once, in a few milliseconds", []() {
        BusRig rig;
        rig.run(1500);
        const uint32_t t0 = rig.now;
        for (int i = 0; i < 3; i++) rig.nodes[i]->queueInput(input(kKey1, hn::ACTION_PRESS, 1));
        rig.run(100);
        EXPECT_EQ(rig.forwarded.size(), static_cast<size_t>(3));
        for (uint32_t t : rig.forwardTimes) EXPECT_TRUE(t - t0 <= 10);
        for (int i = 0; i < 3; i++) EXPECT_TRUE(!rig.nodes[i]->inputsPending());
        EXPECT_EQ(rig.medium.violations, 0);
    });

    addTest(s, "One STATE broadcast serves every slave (local filtering)", []() {
        BusRig rig;
        rig.run(1500);
        CatalogState st;
        for (int i = 0; i < 3; i++) {
            Value v;
            v.kind = hn::VALUE_BOOL;
            v.number = 1;
            st.set(static_cast<uint16_t>(kAaLt + i), v);
        }
        for (auto& p : rig.panels) p.received.clear();
        const int before = rig.medium.stateFrames;
        rig.bridge.sendState(st, st.allIds());
        rig.run(20);
        EXPECT_EQ(rig.medium.stateFrames - before, 1);
        for (int i = 0; i < 3; i++) {
            EXPECT_EQ(rig.panels[i].received.size(), static_cast<size_t>(1));
            EXPECT_EQ(rig.panels[i].received[0].id, static_cast<uint16_t>(kAaLt + i));
        }
    });

    addTest(s, "Unplugged slave is dropped, then rejoins and re-identifies", []() {
        BusRig rig;
        rig.run(1500);
        rig.frozen[1] = true;
        rig.slavePorts[1].muted = true;
        rig.run(500);
        EXPECT_TRUE(!rig.bridge.nodes().at(2).online);
        EXPECT_TRUE(rig.bridge.nodes().at(1).online);
        rig.frozen[1] = false;
        rig.slavePorts[1].muted = false;
        rig.slavePorts[1].rx.clear();
        rig.run(1500);
        EXPECT_TRUE(rig.bridge.nodes().at(2).online);
        EXPECT_TRUE(rig.bridge.nodes().at(2).haveHello);
        EXPECT_TRUE(rig.bridge.takeFullStateDue());
        EXPECT_EQ(rig.medium.violations, 0);
    });

    addTest(s, "Lost bus ACK: slave retries, master de-duplicates", []() {
        BusRig rig;
        rig.run(1500);
        rig.slavePorts[0].dropAcks = 1;
        rig.nodes[0]->queueInput(input(kMasterArm, hn::ACTION_SET_POSITION, 1));
        rig.run(200);
        EXPECT_EQ(rig.slavePorts[0].dropAcks, 0);
        EXPECT_TRUE(rig.master.counters().duplicateInputs >= 1);
        EXPECT_EQ(rig.forwarded.size(), static_cast<size_t>(1));
        EXPECT_TRUE(!rig.nodes[0]->inputsPending());
    });

    addTest(s, "Discovery finds a board missing from the fixed list", []() {
        Medium medium;
        SimPort mb, sb;
        mb.id = 0; mb.medium = &medium;
        sb.id = 1; sb.medium = &medium;
        medium.ports = {&mb, &sb};
        UsbPort usb;
        LinkSession bridge;
        usb.out = [&](const uint8_t* d, size_t n) { bridge.feed(d, n); };
        hn::BusMaster master(usb, mb);
        master.enableDiscovery(true);
        TestPanel p;
        hn::Node node(sb, p);
        node.beginBus(42);
        for (uint32_t t = 1; t < 3000; t++) { master.update(t); node.update(t); }
        EXPECT_EQ(master.slaveCount(), 1);
        EXPECT_TRUE(master.slave(0).online);
        EXPECT_TRUE(bridge.nodes().count(42) == 1);
        EXPECT_EQ(medium.violations, 0);
    });

    addTest(s, "Address changes reject collisions and serialize per slave", []() {
        UsbPort upstream, bus;
        hn::FrameDecoder upstreamDecoder, busDecoder;
        struct CapturedFrame {
            hn::FrameHeader header;
            std::vector<uint8_t> payload;
        };
        std::vector<CapturedFrame> upstreamFrames, busFrames;
        const auto capture = [](hn::FrameDecoder& decoder, std::vector<CapturedFrame>& frames,
                                const uint8_t* bytes, size_t len) {
            for (size_t i = 0; i < len; i++) {
                if (decoder.feed(bytes[i]) != hn::FrameDecoder::Result::Frame) continue;
                CapturedFrame f;
                f.header = decoder.header();
                f.payload.assign(decoder.payload(), decoder.payload() + f.header.len);
                frames.push_back(f);
            }
        };
        upstream.out = [&](const uint8_t* d, size_t n) { capture(upstreamDecoder, upstreamFrames, d, n); };
        bus.out = [&](const uint8_t* d, size_t n) { capture(busDecoder, busFrames, d, n); };
        hn::BusMaster master(upstream, bus);
        EXPECT_TRUE(master.addSlave(1));
        EXPECT_TRUE(master.addSlave(2));

        const auto requestAddress = [&](uint8_t dst, uint8_t seq, uint8_t address) {
            const auto f = frame(dst, hn::ADDR_BRIDGE, hn::MSG_CONFIG, seq, {hn::CFG_BUS_ADDRESS, address});
            upstream.rx.insert(upstream.rx.end(), f.begin(), f.end());
        };
        requestAddress(1, 1, 2);
        master.update(1);
        EXPECT_TRUE(master.slave(0).pendingAddress == 0);
        EXPECT_TRUE(std::none_of(busFrames.begin(), busFrames.end(), [](const CapturedFrame& f) {
            return f.header.type == hn::MSG_CONFIG;
        }));
        EXPECT_TRUE(std::any_of(upstreamFrames.begin(), upstreamFrames.end(), [](const CapturedFrame& f) {
            return f.header.type == hn::MSG_NACK && f.payload.size() == 4 &&
                   f.payload[0] == 1 && f.payload[1] == hn::NACK_BAD_VALUE;
        }));

        requestAddress(1, 2, 3);
        requestAddress(1, 3, 4);
        master.update(2);
        EXPECT_EQ(master.slave(0).pendingAddress, 3);
        EXPECT_EQ(master.slave(0).pendingAddressSeq, 2);
        EXPECT_TRUE(std::any_of(upstreamFrames.begin(), upstreamFrames.end(), [](const CapturedFrame& f) {
            return f.header.type == hn::MSG_NACK && f.payload.size() == 4 &&
                   f.payload[0] == 3 && f.payload[1] == hn::NACK_BAD_VALUE;
        }));

        requestAddress(2, 4, 3);
        master.update(3);
        EXPECT_EQ(master.slave(1).pendingAddress, 0);
        EXPECT_TRUE(std::any_of(upstreamFrames.begin(), upstreamFrames.end(), [](const CapturedFrame& f) {
            return f.header.type == hn::MSG_NACK && f.payload.size() == 4 &&
                   f.payload[0] == 4 && f.payload[1] == hn::NACK_BAD_VALUE;
        }));

        const auto pollReply = frame(hn::ADDR_LINK_NODE, 2, hn::MSG_POLL_EMPTY, 0, {});
        bus.rx.insert(bus.rx.end(), pollReply.begin(), pollReply.end());
        master.update(4);
        EXPECT_TRUE(std::any_of(busFrames.begin(), busFrames.end(), [](const CapturedFrame& f) {
            return f.header.type == hn::MSG_CONFIG && f.header.dst == 1;
        }));

        const auto ack = frame(hn::ADDR_LINK_NODE, 1, hn::MSG_ACK, 0, {2});
        bus.rx.insert(bus.rx.end(), ack.begin(), ack.end());
        master.update(5);
        EXPECT_EQ(master.slave(0).address, 3);
        EXPECT_EQ(master.slave(0).pendingAddress, 0);

        const auto otherPollReply = frame(hn::ADDR_LINK_NODE, 2, hn::MSG_POLL_EMPTY, 0, {});
        bus.rx.insert(bus.rx.end(), otherPollReply.begin(), otherPollReply.end());
        master.update(6);
        EXPECT_TRUE(std::any_of(busFrames.begin(), busFrames.end(), [](const CapturedFrame& f) {
            return f.header.type == hn::MSG_POLL && f.header.dst == 3;
        }));
    });

    addTest(s, "Bus reply queue handles the master's maximum downstream burst", []() {
        BusRig rig;
        rig.run(1500);
        hn::FrameDecoder decoder;
        const auto previousOutput = rig.masterUsb.out;
        uint8_t modeAcks = 0;
        rig.masterUsb.out = [&](const uint8_t* bytes, size_t len) {
            for (size_t i = 0; i < len; i++)
                if (decoder.feed(bytes[i]) == hn::FrameDecoder::Result::Frame &&
                    decoder.header().type == hn::MSG_MODE_ACK)
                    modeAcks++;
            previousOutput(bytes, len);
        };
        for (uint8_t i = 0; i < hn::BusMaster::kQueueFrames; i++) {
            const uint8_t mode = i & 1 ? hn::MODE_LAMP_TEST : hn::MODE_SIM;
            const auto request = frame(1, hn::ADDR_BRIDGE, hn::MSG_MODE, i, {mode});
            rig.masterUsb.rx.insert(rig.masterUsb.rx.end(), request.begin(), request.end());
        }
        rig.run(300);
        EXPECT_EQ(modeAcks, hn::BusMaster::kQueueFrames);
        EXPECT_EQ(rig.panels[0].mode, hn::MODE_LAMP_TEST);
    });

    addTest(s, "Discovery conflict probe identifies already-polled duplicate addresses", []() {
        Medium medium;
        SimPort masterBus, firstBus, secondBus;
        masterBus.id = 0; masterBus.medium = &medium;
        firstBus.id = 1; firstBus.medium = &medium;
        secondBus.id = 2; secondBus.medium = &medium;
        medium.ports = {&masterBus, &firstBus, &secondBus};
        UsbPort upstream;
        hn::BusMaster master(upstream, masterBus);
        EXPECT_TRUE(master.addSlave(42));

        TestPanel firstPanel, secondPanel;
        firstPanel.boardId[0] = 1;
        secondPanel.boardId[0] = 2;
        hn::Node first(firstBus, firstPanel), second(secondBus, secondPanel);
        first.beginBus(42);
        second.beginBus(42);
        first.seedRandom(0x1234);
        second.seedRandom(0x5678);

        for (uint32_t t = 1; t <= 500; t++) {
            master.update(t);
            first.update(t);
            second.update(t);
        }
        master.enableDiscovery(true);
        for (uint32_t t = 501; t <= 4000; t++) {
            master.update(t);
            first.update(t);
            second.update(t);
        }
        EXPECT_TRUE(master.counters().conflicts >= 1);
    });
}

} // namespace

void registerHornetV2Tests() {
    framingTests();
    recordTests();
    catalogTests();
    filterTests();
    elementTests();
    nativeTests();
    usbTests();
    busTests();
}
