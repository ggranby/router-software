// Behavioural tests for the protocol core: handshake parsing, import-line
// parsing and validation, state-map bounds, and log formatting.

#include "BiosProtocol.hpp"
#include "ControlDatabase.hpp"
#include "DeviceRegistry.hpp"
#include "test_framework.hpp"

#include <cstring>
#include <string>
#include <vector>

using namespace dcsbios;

namespace {

/// Feed @p bytes into @p parser; return the result after the last byte.
HandshakeParser::Result feed(HandshakeParser& parser, const std::vector<uint8_t>& bytes) {
    HandshakeParser::Result r = HandshakeParser::Result::Pending;
    for (uint8_t b : bytes) r = parser.processByte(b);
    return r;
}

std::vector<uint8_t> pongHeader(uint8_t flags, const std::string& name) {
    std::vector<uint8_t> v = {0xAA, 0xDE, 0xAD, 0x02, flags, static_cast<uint8_t>(name.size())};
    v.insert(v.end(), name.begin(), name.end());
    return v;
}

void appendSub(std::vector<uint8_t>& v, uint16_t addr, uint16_t mask, uint8_t shift) {
    v.push_back(static_cast<uint8_t>(addr & 0xFF));
    v.push_back(static_cast<uint8_t>(addr >> 8));
    v.push_back(static_cast<uint8_t>(mask & 0xFF));
    v.push_back(static_cast<uint8_t>(mask >> 8));
    v.push_back(shift);
}

std::vector<ImportCommand> parseImport(const std::string& text) {
    std::vector<ImportCommand> out;
    ImportLineParser p;
    p.onCommand = [&](const ImportCommand& c) { out.push_back(c); };
    p.processBytes(reinterpret_cast<const uint8_t*>(text.data()), text.size());
    return out;
}

} // namespace

void registerCoreTests() {
    // ── Handshake ────────────────────────────────────────────────────────────
    auto hs = createSuite("HandshakeParser - pong decoding");

    addTest(hs, "Standalone bidir device with subscriptions", []() {
        auto bytes = pongHeader(0x04, "UFC");
        bytes.push_back(2); bytes.push_back(0);
        appendSub(bytes, 0x7406, 0xFFFF, 0);
        appendSub(bytes, 0x740A, 0x00FF, 0);
        HandshakeParser p;
        EXPECT_TRUE(feed(p, bytes) == HandshakeParser::Result::Complete);
        DeviceInfo d;
        p.populateDevice(d);
        EXPECT_TRUE(d.deviceName == "UFC");
        EXPECT_TRUE(d.role == DeviceRole::Standalone);
        EXPECT_TRUE(d.bidir);
        EXPECT_TRUE(!d.wantsAll);
        EXPECT_TRUE(d.wantsAddress(0x7406));
        EXPECT_TRUE(!d.wantsAddress(0x0000));
    });

    addTest(hs, "Wildcard subscription means full stream", []() {
        auto bytes = pongHeader(0x00, "");
        bytes.push_back(1); bytes.push_back(0);
        appendSub(bytes, 0xFFFF, 0xFFFF, 0);
        HandshakeParser p;
        EXPECT_TRUE(feed(p, bytes) == HandshakeParser::Result::Complete);
        DeviceInfo d;
        p.populateDevice(d);
        EXPECT_TRUE(d.wantsAll);
    });

    addTest(hs, "Master with subscriptions and empty slave list", []() {
        auto bytes = pongHeader(0x05, "1A1-MASTER");
        bytes.push_back(1); bytes.push_back(0);
        appendSub(bytes, 0x1000, 0xFFFF, 0);
        bytes.push_back(0); // slave_count
        HandshakeParser p;
        EXPECT_TRUE(feed(p, bytes) == HandshakeParser::Result::Complete);
        DeviceInfo d;
        p.populateDevice(d);
        EXPECT_TRUE(d.role == DeviceRole::RS485Master);
        EXPECT_TRUE(d.slaves.empty());
    });

    addTest(hs, "Master without the slave_count byte is not complete", []() {
        auto bytes = pongHeader(0x01, "M");
        bytes.push_back(1); bytes.push_back(0);
        appendSub(bytes, 0x1000, 0xFFFF, 0);
        HandshakeParser p;
        EXPECT_TRUE(feed(p, bytes) == HandshakeParser::Result::Pending);
    });

    addTest(hs, "Master with zero subscriptions still reads slave list", []() {
        auto bytes = pongHeader(0x01, "M");
        bytes.push_back(0); bytes.push_back(0); // sub_count = 0
        HandshakeParser p;
        EXPECT_TRUE(feed(p, bytes) == HandshakeParser::Result::Pending);
        EXPECT_TRUE(p.processByte(0) == HandshakeParser::Result::Complete); // slave_count
    });

    addTest(hs, "Slave with zero subscriptions does not swallow next slave", []() {
        auto bytes = pongHeader(0x01, "M");
        bytes.push_back(0); bytes.push_back(0); // master subs
        bytes.push_back(2);                     // two slaves
        // slave 1: addr 0x01, name "A", 0 subs
        bytes.insert(bytes.end(), {0x01, 1, 'A', 0, 0});
        // slave 2: addr 0x02, name "B", 1 sub
        bytes.insert(bytes.end(), {0x02, 1, 'B', 1, 0});
        appendSub(bytes, 0x2000, 0xFFFF, 0);
        HandshakeParser p;
        EXPECT_TRUE(feed(p, bytes) == HandshakeParser::Result::Complete);
        DeviceInfo d;
        p.populateDevice(d);
        EXPECT_EQ(d.slaves.size(), size_t(2));
        EXPECT_EQ(d.slaves[0].busAddress, 0x01);
        EXPECT_EQ(d.slaves[1].busAddress, 0x02);
        EXPECT_TRUE(d.slaves[1].name == "B");
        EXPECT_EQ(d.slaves[1].subs.size(), size_t(1));
    });

    addTest(hs, "Wrong frame type fails", []() {
        HandshakeParser p;
        EXPECT_TRUE(feed(p, {0xAA, 0xDE, 0xAD, 0x03}) == HandshakeParser::Result::Failed);
    });

    addTest(hs, "Leading noise before header is ignored", []() {
        auto bytes = std::vector<uint8_t>{0x00, 0xAA, 0x13, 0x55};
        auto pong = pongHeader(0x00, "X");
        bytes.insert(bytes.end(), pong.begin(), pong.end());
        bytes.push_back(0); bytes.push_back(0);
        HandshakeParser p;
        EXPECT_TRUE(feed(p, bytes) == HandshakeParser::Result::Complete);
    });

    // ── Import lines ─────────────────────────────────────────────────────────
    auto imp = createSuite("ImportLineParser - parsing and validation");

    addTest(imp, "Action-only and valued commands", []() {
        auto cmds = parseImport("UFC_OPTION1 TOGGLE\nUFC_COMM1 SET_STATE 3\r\n");
        EXPECT_EQ(cmds.size(), size_t(2));
        EXPECT_TRUE(cmds[0].identifier == "UFC_OPTION1" && cmds[0].action == "TOGGLE" && cmds[0].value.empty());
        EXPECT_TRUE(cmds[1].value == "3");
        EXPECT_TRUE(cmds[1].toLine() == "UFC_COMM1 SET_STATE 3\n");
    });

    addTest(imp, "Line without action is rejected", []() {
        EXPECT_TRUE(parseImport("JUSTONEWORD\n").empty());
    });

    addTest(imp, "Invalid identifier characters are rejected", []() {
        EXPECT_TRUE(parseImport("BAD-ID TOGGLE\n").empty());
        EXPECT_TRUE(parseImport("BAD\x01ID TOGGLE\n").empty());
    });

    addTest(imp, "Control bytes in value are rejected", []() {
        EXPECT_TRUE(parseImport(std::string("ID SET_STATE a\x1b") + "b\n").empty());
    });

    addTest(imp, "Overlong line is dropped, not truncated", []() {
        std::string longLine = "ID SET_STATE " + std::string(kImportLineMaxBytes, '1') + "\n";
        ImportLineParser p;
        int count = 0;
        p.onCommand = [&](const ImportCommand&) { ++count; };
        p.processBytes(reinterpret_cast<const uint8_t*>(longLine.data()), longLine.size());
        EXPECT_EQ(count, 0);
        EXPECT_EQ(p.rejectedLines(), uint64_t(1));
        // Parser recovers for the next line.
        const char next[] = "ID TOGGLE\n";
        p.processBytes(reinterpret_cast<const uint8_t*>(next), std::strlen(next));
        EXPECT_EQ(count, 1);
    });

    // ── State map bounds ─────────────────────────────────────────────────────
    auto st = createSuite("BiosStateMap - bounds");

    addTest(st, "Write past end of address space is clipped", []() {
        BiosStateMap m;
        uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        m.write(0xFFFE, data, sizeof(data));
        EXPECT_EQ(m.readWord(0xFFFE), 0x0201);
        auto dirty = m.takeDirty();
        EXPECT_EQ(dirty.size(), size_t(1));
        EXPECT_EQ(dirty[0], 0xFFFE);
    });

    addTest(st, "ExportParser record near end of space does not overflow", []() {
        BiosStateMap m;
        ExportParser parser(m);
        std::vector<uint8_t> in = {0x55, 0x55, 0x55, 0x55, 0xFE, 0xFF, 0x10, 0x00};
        for (int i = 0; i < 16; ++i) in.push_back(static_cast<uint8_t>(0xA0 + i));
        parser.processBytes(in.data(), in.size());
        EXPECT_EQ(m.readWord(0xFFFE), 0xA1A0);
    });

    addTest(st, "Odd top address reads as zero high byte", []() {
        BiosStateMap m;
        uint8_t data[2] = {0x7F, 0x00};
        m.write(0xFFFE, data, 2);
        EXPECT_EQ(m.readWord(0xFFFF), 0x0000);
        EXPECT_EQ(m.byteAt(0x10000), 0);
    });

    // ── Formatting ───────────────────────────────────────────────────────────
    auto fmt = createSuite("FormatWireStateChange");

    addTest(fmt, "Integer control", []() {
        BiosStateMap m;
        uint8_t data[2] = {0x34, 0x12};
        m.write(0x1000, data, 2);
        ControlDescriptor d;
        d.identifier = "TEST_CTRL";
        d.byteAddr = 0x1000; d.mask = 0x00FF; d.shift = 0;
        EXPECT_TRUE(FormatWireStateChange(d, m) == L"TEST_CTRL SET_STATE 52");
    });

    addTest(fmt, "Long string control does not overflow", []() {
        BiosStateMap m;
        std::vector<uint8_t> text(600, 'x');
        m.write(0x2000, text.data(), static_cast<uint16_t>(text.size()));
        ControlDescriptor d;
        d.identifier = std::string(600, 'I');
        d.byteAddr = 0x2000; d.isString = true; d.strLen = 600;
        auto line = FormatWireStateChange(d, m);
        EXPECT_EQ(line.size(), size_t(600 + 11 + 600 + 2));
    });

    addTest(fmt, "String control at top of address space stops at boundary", []() {
        BiosStateMap m;
        uint8_t data[2] = {'A', 'B'};
        m.write(0xFFFE, data, 2);
        ControlDescriptor d;
        d.identifier = "S";
        d.byteAddr = 0xFFFE; d.isString = true; d.strLen = 16;
        EXPECT_TRUE(FormatWireStateChange(d, m) == L"S SET_STATE \"AB\"");
    });
}
