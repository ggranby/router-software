#pragma once
/**
 * @file HornetNative.hpp
 * @brief Bridge-side logic for the Hornet-native path (protocol v2 + native exporter).
 *
 * Portable C++17 with no Windows dependency, so it is unit-tested and fuzzed
 * on Linux. It shares the protocol code with the Arduino library
 * (libraries/HornetLink/src), so both ends are always built from the same
 * definitions.
 *
 * Pieces:
 *  - CatalogState          sim state keyed by catalogue id (replaces the 64 KiB map)
 *  - parseExporterDatagram reads HornetLinkNative.lua datagrams
 *  - formatExporterInput   typed input -> exporter text line ("0101 SET 1")
 *  - validateInput         catalogue check -> NACK reason
 *  - SyncTracker           DCS-is-truth discrepancy list for the spawn overlay
 *  - decodeFrame           human-readable one-line decode for logs / sniffer view
 *  - LinkSession           one USB link speaking protocol v2 (panel or bus master)
 *
 * Wiring this into main.cpp (source selector, decoder view, settings UI) is
 * tracked in docs/DEVELOPER_GUIDE.md.
 */

#include "HornetCatalog.h"
#include "protocol/HnRecords.h"

#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace hornet_native {

// ─────────────────────────────────────────────────────────────────────────────
// Catalogue helpers
// ─────────────────────────────────────────────────────────────────────────────

/// "UFC.KEY_1" (or "0xPPCC" when unknown).
inline std::string controlName(uint16_t id) {
    char buf[48];
    Hornet::controlName(id, buf, sizeof(buf));
    return buf;
}

/// Position / value as text: "ARM", "PRESSED", "32768", "\"1234\"".
inline std::string valueText(uint16_t id, uint16_t value) {
    Hornet::ControlInfo row{};
    char buf[24];
    if (Hornet::findControl(id, row)) {
        Hornet::positionName(row, value, buf, sizeof(buf));
        return buf;
    }
    return std::to_string(value);
}

/// Wire value kind for a catalogue kind.
inline uint8_t valueKindFor(uint8_t kind) {
    switch (static_cast<Hornet::Kind>(kind)) {
    case Hornet::Kind::Lamp: return hn::VALUE_BOOL;
    case Hornet::Kind::Axis:
    case Hornet::Kind::Gauge: return hn::VALUE_ANALOG;
    case Hornet::Kind::Text: return hn::VALUE_TEXT;
    default: return hn::VALUE_POSITION;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CatalogState
// ─────────────────────────────────────────────────────────────────────────────

struct Value {
    uint8_t kind = hn::VALUE_POSITION;
    uint16_t number = 0;
    std::string text;
    bool operator==(const Value& o) const { return kind == o.kind && number == o.number && text == o.text; }
    bool operator!=(const Value& o) const { return !(*this == o); }
};

/// Sim state keyed by catalogue id, with a dirty set for change-only updates.
class CatalogState {
public:
    /// Returns true if the value changed.
    bool set(uint16_t id, const Value& v) {
        auto it = values_.find(id);
        if (it != values_.end() && it->second == v) return false;
        values_[id] = v;
        dirty_.insert(id);
        return true;
    }
    const Value* get(uint16_t id) const {
        auto it = values_.find(id);
        return it == values_.end() ? nullptr : &it->second;
    }
    std::vector<uint16_t> takeDirty() {
        std::vector<uint16_t> out(dirty_.begin(), dirty_.end());
        dirty_.clear();
        return out;
    }
    std::vector<uint16_t> allIds() const {
        std::vector<uint16_t> out;
        out.reserve(values_.size());
        for (const auto& kv : values_) out.push_back(kv.first);
        return out;
    }
    size_t size() const { return values_.size(); }
    void clear() { values_.clear(); dirty_.clear(); }

private:
    std::map<uint16_t, Value> values_;
    std::set<uint16_t> dirty_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Exporter datagrams (HornetLinkNative.lua -> bridge)
// ─────────────────────────────────────────────────────────────────────────────

struct DatagramHeader {
    uint32_t catalogHash = 0;
    std::string aircraft;
    bool active = false;
    bool full = false;
};

struct ParseStats {
    uint32_t unknownIds = 0;
    uint32_t badLines = 0;
};

enum class ParseResult { Ok, NotHornetLink, BadHeader };

namespace detail {
inline int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
inline bool parseUInt(std::string_view s, uint32_t& out, int base = 10, uint32_t max = 0xFFFFFFFFu) {
    if (s.empty() || s.size() > 10) return false;
    uint64_t v = 0;
    for (char c : s) {
        const int d = base == 16 ? hexVal(c) : (c >= '0' && c <= '9' ? c - '0' : -1);
        if (d < 0) return false;
        v = v * static_cast<uint64_t>(base) + static_cast<uint64_t>(d);
        if (v > max) return false;
    }
    out = static_cast<uint32_t>(v);
    return true;
}
inline std::string unescape(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '%' && i + 2 < s.size()) {
            const int hi = hexVal(s[i + 1]), lo = hexVal(s[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(s[i]);
    }
    return out;
}
} // namespace detail

/**
 * @brief Parse one datagram from the native exporter into @p state.
 *
 * Header: "HLN 2 0x<hash> <aircraft> <active> <full>". Body lines
 * "PPCC=<n>" or "PPCC=T:<escaped text>". Unknown ids and malformed lines are
 * counted and skipped; they never abort the datagram.
 */
inline ParseResult parseExporterDatagram(std::string_view data, CatalogState& state,
                                         DatagramHeader& header, ParseStats& stats) {
    const size_t eol = data.find('\n');
    std::string_view head = data.substr(0, eol);
    if (head.substr(0, 6) != "HLN 2 ") return ParseResult::NotHornetLink;

    // Split the header on spaces: HLN 2 hash aircraft active full
    std::vector<std::string_view> f;
    size_t pos = 0;
    while (pos <= head.size() && f.size() < 7) {
        size_t sp = head.find(' ', pos);
        if (sp == std::string_view::npos) sp = head.size();
        f.push_back(head.substr(pos, sp - pos));
        pos = sp + 1;
    }
    if (f.size() != 6) return ParseResult::BadHeader;
    std::string_view hash = f[2];
    if (hash.substr(0, 2) == "0x" || hash.substr(0, 2) == "0X") hash.remove_prefix(2);
    uint32_t h = 0, active = 0, full = 0;
    if (!detail::parseUInt(hash, h, 16) || !detail::parseUInt(f[4], active, 10, 1) ||
        !detail::parseUInt(f[5], full, 10, 1) || f[3].empty() || f[3].size() > 64)
        return ParseResult::BadHeader;
    header.catalogHash = h;
    header.aircraft = std::string(f[3]);
    header.active = active != 0;
    header.full = full != 0;

    if (eol == std::string_view::npos) return ParseResult::Ok;
    std::string_view body = data.substr(eol + 1);
    while (!body.empty()) {
        size_t nl = body.find('\n');
        std::string_view line = body.substr(0, nl);
        body = nl == std::string_view::npos ? std::string_view() : body.substr(nl + 1);
        if (line.empty()) continue;
        uint32_t id = 0;
        if (line.size() < 6 || line[4] != '=' || !detail::parseUInt(line.substr(0, 4), id, 16, 0xFFFF)) {
            stats.badLines++;
            continue;
        }
        Hornet::ControlInfo row{};
        if (!Hornet::findControl(static_cast<uint16_t>(id), row)) { stats.unknownIds++; continue; }
        std::string_view val = line.substr(5);
        Value v;
        v.kind = valueKindFor(row.kind);
        if (v.kind == hn::VALUE_TEXT) {
            if (val.substr(0, 2) != "T:") { stats.badLines++; continue; }
            v.text = detail::unescape(val.substr(2));
            if (v.text.size() > row.count) v.text.resize(row.count);
            while (v.text.size() < row.count) v.text.insert(v.text.begin(), ' ');
        } else {
            uint32_t n = 0;
            if (!detail::parseUInt(val, n, 10, 65535)) { stats.badLines++; continue; }
            if (v.kind == hn::VALUE_POSITION && row.count && n >= row.count) { stats.badLines++; continue; }
            if (v.kind == hn::VALUE_BOOL) n = n ? 1 : 0;
            v.number = static_cast<uint16_t>(n);
        }
        state.set(static_cast<uint16_t>(id), v);
    }
    return ParseResult::Ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Inputs (bridge -> exporter)
// ─────────────────────────────────────────────────────────────────────────────

/// Check an input against the catalogue. Returns 0 or a hn::NACK_* reason.
inline uint8_t validateInput(const hn::InputRecord& r) {
    Hornet::ControlInfo row{};
    if (!Hornet::findControl(r.id, row)) return hn::NACK_UNKNOWN_CONTROL;
    if (!(row.flags & Hornet::kInfoInput)) return hn::NACK_BAD_VALUE;
    const auto kind = static_cast<Hornet::Kind>(row.kind);
    switch (r.action) {
    case hn::ACTION_SET_POSITION:
        if (kind == Hornet::Kind::Axis || kind == Hornet::Kind::Rotary) return hn::NACK_BAD_VALUE;
        return r.arg < row.count ? 0 : hn::NACK_BAD_VALUE;
    case hn::ACTION_PRESS:
    case hn::ACTION_RELEASE:
        return kind == Hornet::Kind::Button ? 0 : hn::NACK_BAD_VALUE;
    case hn::ACTION_ANALOG:
        return kind == Hornet::Kind::Axis ? 0 : hn::NACK_BAD_VALUE;
    case hn::ACTION_STEP:
        return (kind == Hornet::Kind::Rotary || kind == Hornet::Kind::Selector ||
                kind == Hornet::Kind::Switch || kind == Hornet::Kind::Axis) ? 0 : hn::NACK_BAD_VALUE;
    default:
        return hn::NACK_BAD_VALUE;
    }
}

/// Exporter line for a (validated) input: "0101 SET 1", "0221 STEP -2", "0202 PRESS".
inline std::string formatExporterInput(const hn::InputRecord& r) {
    char buf[32];
    switch (r.action) {
    case hn::ACTION_SET_POSITION: std::snprintf(buf, sizeof(buf), "%04X SET %u", r.id, r.arg); break;
    case hn::ACTION_STEP: std::snprintf(buf, sizeof(buf), "%04X STEP %+d", r.id, r.step()); break;
    case hn::ACTION_PRESS: std::snprintf(buf, sizeof(buf), "%04X PRESS", r.id); break;
    case hn::ACTION_RELEASE: std::snprintf(buf, sizeof(buf), "%04X RELEASE", r.id); break;
    case hn::ACTION_ANALOG: std::snprintf(buf, sizeof(buf), "%04X ANALOG %u", r.id, r.arg); break;
    default: return {};
    }
    return buf;
}

/// "UFC.KEY_1 PRESS", "MASTER_ARM.MASTER_ARM=ARM" - for logs and the wiring test.
inline std::string describeInput(const hn::InputRecord& r) {
    const std::string name = controlName(r.id);
    switch (r.action) {
    case hn::ACTION_SET_POSITION: return name + "=" + valueText(r.id, r.arg);
    case hn::ACTION_STEP: return name + (r.step() >= 0 ? " STEP +" : " STEP ") + std::to_string(r.step());
    case hn::ACTION_PRESS: return name + " PRESS";
    case hn::ACTION_RELEASE: return name + " RELEASE";
    case hn::ACTION_ANALOG: return name + "=" + std::to_string(r.arg);
    default: return name + " ?";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Sync (DCS is the source of truth)
// ─────────────────────────────────────────────────────────────────────────────

struct Discrepancy {
    uint16_t id = 0;
    std::string name;    ///< "MASTER_ARM.MASTER_ARM"
    std::string cockpit; ///< physical position, e.g. "ARM"
    std::string sim;     ///< DCS position, e.g. "SAFE"
};

/**
 * @brief Compares physical switch positions with DCS.
 *
 * DCS always wins: the bridge never pushes cockpit positions into the sim on
 * connect or spawn. Instead the pilot gets a list of switches that disagree
 * (in the bridge window and, when enabled, the in-game overlay) until each one
 * is moved to match. Momentary buttons are not compared.
 */
class SyncTracker {
public:
    bool overlayEnabled = true;       ///< user setting: show the discrepancy overlay
    uint16_t analogTolerance = 4096;  ///< pots within ~6 % count as matching

    void setCockpit(uint16_t id, uint16_t value) { cockpit_[id] = value; }
    void forgetAll() { cockpit_.clear(); }

    std::vector<Discrepancy> discrepancies(const CatalogState& sim) const {
        std::vector<Discrepancy> out;
        for (const auto& kv : cockpit_) {
            Hornet::ControlInfo row{};
            if (!Hornet::findControl(kv.first, row)) continue;
            const auto kind = static_cast<Hornet::Kind>(row.kind);
            if (kind == Hornet::Kind::Button || kind == Hornet::Kind::Rotary) continue;
            const Value* v = sim.get(kv.first);
            if (!v) continue;
            bool differs;
            if (kind == Hornet::Kind::Axis) {
                const int d = static_cast<int>(v->number) - static_cast<int>(kv.second);
                differs = (d < 0 ? -d : d) > analogTolerance;
            } else {
                differs = v->number != kv.second;
            }
            if (!differs) continue;
            Discrepancy dsc;
            dsc.id = kv.first;
            dsc.name = controlName(kv.first);
            dsc.cockpit = kind == Hornet::Kind::Axis ? std::to_string(kv.second) : valueText(kv.first, kv.second);
            dsc.sim = kind == Hornet::Kind::Axis ? std::to_string(v->number) : valueText(kv.first, v->number);
            out.push_back(std::move(dsc));
        }
        return out;
    }

    /// Text for the overlay / status panel. Empty when everything matches or the overlay is off.
    std::string overlayText(const CatalogState& sim) const {
        if (!overlayEnabled) return {};
        const auto list = discrepancies(sim);
        if (list.empty()) return {};
        std::string s = "COCKPIT DOES NOT MATCH DCS (" + std::to_string(list.size()) + ")\n";
        for (const auto& d : list) s += d.name + ": set " + d.sim + " (is " + d.cockpit + ")\n";
        return s;
    }

private:
    std::map<uint16_t, uint16_t> cockpit_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Subscriptions and STATE frames
// ─────────────────────────────────────────────────────────────────────────────

struct Subscription {
    uint8_t scope = hn::SUBSCRIBE_ALL;
    std::set<uint16_t> items; ///< control ids or panel ids (high byte) depending on scope

    bool wants(uint16_t id) const {
        switch (scope) {
        case hn::SUBSCRIBE_ALL: return true;
        case hn::SUBSCRIBE_CONTROLS: return items.count(id) != 0;
        case hn::SUBSCRIBE_PANELS: return items.count(static_cast<uint16_t>(id & 0xFF00)) != 0;
        default: return false;
        }
    }

    static bool parse(const uint8_t* p, size_t n, Subscription& out) {
        if (n < 1 || (n - 1) % 2 != 0 || p[0] > hn::SUBSCRIBE_ALL) return false;
        out.scope = p[0];
        out.items.clear();
        for (size_t i = 1; i + 1 < n; i += 2) out.items.insert(hn::getU16(p + i));
        return true;
    }
};

/// Encode STATE frames (one or more) for @p ids. Text/values come from @p state.
inline std::vector<std::vector<uint8_t>> buildStateFrames(const CatalogState& state,
                                                          const std::vector<uint16_t>& ids,
                                                          uint8_t dst, uint8_t& seq) {
    std::vector<std::vector<uint8_t>> frames;
    uint8_t payload[hn::kMaxPayload];
    hn::StateWriter w(payload, sizeof(payload));
    auto flush = [&]() {
        if (!w.size()) return;
        hn::FrameHeader h;
        h.dst = dst;
        h.src = hn::ADDR_BRIDGE;
        h.type = hn::MSG_STATE;
        h.seq = seq++;
        std::vector<uint8_t> wire(hn::kMaxWireFrame);
        wire.resize(hn::encodeFrame(h, payload, w.size(), wire.data(), wire.size()));
        frames.push_back(std::move(wire));
        w.clear();
    };
    auto add = [&](uint16_t id, const Value& v) {
        switch (v.kind) {
        case hn::VALUE_BOOL: return w.addBool(id, v.number != 0);
        case hn::VALUE_ANALOG: return w.addAnalog(id, v.number);
        case hn::VALUE_TEXT:
            return w.addText(id, reinterpret_cast<const uint8_t*>(v.text.data()),
                             static_cast<uint8_t>(v.text.size() > 64 ? 64 : v.text.size()));
        default: return w.addPosition(id, static_cast<uint8_t>(v.number));
        }
    };
    for (uint16_t id : ids) {
        const Value* v = state.get(id);
        if (!v) continue;
        if (!add(id, *v)) { flush(); add(id, *v); }
    }
    flush();
    return frames;
}

// ─────────────────────────────────────────────────────────────────────────────
// Frame decoder for logs / sniffer view
// ─────────────────────────────────────────────────────────────────────────────

inline std::string addressName(uint8_t a) {
    switch (a) {
    case hn::ADDR_BROADCAST: return "ALL";
    case hn::ADDR_BRIDGE: return "BRIDGE";
    case hn::ADDR_LINK_NODE: return "LINK";
    case hn::ADDR_UNASSIGNED: return "UNASSIGNED";
    default: return "BUS" + std::to_string(a);
    }
}

/// One readable line, e.g. "BUS3 -> BRIDGE INPUT #7: UFC.KEY_1 PRESS".
inline std::string decodeFrame(const hn::FrameHeader& h, const uint8_t* p, size_t n) {
    std::string s = addressName(h.src) + " -> " + addressName(h.dst) + " " + hn::msgTypeName(h.type) +
                    " #" + std::to_string(h.seq);
    std::string detail;
    switch (h.type) {
    case hn::MSG_INPUT: {
        hn::InputReader rd(p, n);
        hn::InputRecord r;
        while (rd.next(r)) detail += (detail.empty() ? "" : ", ") + describeInput(r);
        if (rd.error()) detail += " [malformed]";
        break;
    }
    case hn::MSG_STATE: {
        hn::StateReader rd(p, n);
        hn::StateRecord r;
        while (rd.next(r)) {
            if (!detail.empty()) detail += ", ";
            detail += controlName(r.id) + "=";
            if (r.kind == hn::VALUE_TEXT) detail += "\"" + std::string(reinterpret_cast<const char*>(r.text), r.textLen) + "\"";
            else if (r.kind == hn::VALUE_POSITION) detail += valueText(r.id, r.value);
            else detail += std::to_string(r.value);
        }
        if (rd.error()) detail += " [malformed]";
        break;
    }
    case hn::MSG_HELLO: {
        hn::Hello hello;
        if (hn::readHello(p, n, hello)) {
            char hash[16];
            std::snprintf(hash, sizeof(hash), "0x%08X", static_cast<unsigned>(hello.catalogHash));
            detail = std::string("\"") + hello.name + "\" fw " + std::to_string(hello.fwMajor) + "." +
                     std::to_string(hello.fwMinor) + " catalogue " + hash +
                     (hello.catalogHash == Hornet::kCatalogHash ? "" : " (MISMATCH)");
        }
        break;
    }
    case hn::MSG_NACK:
        if (n >= 4) detail = std::string(hn::nackName(p[1])) + " for #" + std::to_string(p[0]) + " " + controlName(hn::getU16(p + 2));
        break;
    case hn::MSG_ACK:
        if (n >= 1) detail = "#" + std::to_string(p[0]);
        break;
    case hn::MSG_BUS_EVENT:
        if (n >= 2) detail = std::string(p[0] == hn::BUS_JOIN ? "JOIN " : p[0] == hn::BUS_DROP ? "DROP " : "CONFLICT ") + addressName(p[1]);
        break;
    case hn::MSG_SYNC_REPORT: {
        hn::SyncReader rd(p, n);
        hn::SyncRecord r;
        while (rd.next(r)) detail += (detail.empty() ? "" : ", ") + controlName(r.id) + "=" + valueText(r.id, r.value);
        break;
    }
    default:
        break;
    }
    return detail.empty() ? s : s + ": " + detail;
}

// ─────────────────────────────────────────────────────────────────────────────
// LinkSession: one USB link speaking protocol v2
// ─────────────────────────────────────────────────────────────────────────────

struct NodeInfo {
    uint8_t address = 0;
    bool online = false;
    bool haveHello = false;
    bool catalogMismatch = false;
    hn::Hello hello;
    Subscription subscription;
};

/**
 * @brief Host side of one v2 USB link (a standalone panel or a bus master).
 *
 * Feed it the bytes read from the COM port; it answers with frames through
 * @c write and reports cockpit inputs through @c onInput. It knows every node
 * behind the link (the master's JOIN events plus each node's HELLO), their
 * subscriptions, and flags catalogue-hash mismatches.
 */
class LinkSession {
public:
    std::function<void(const uint8_t*, size_t)> write;              ///< bytes to the COM port
    std::function<void(uint8_t src, const hn::InputRecord&)> onInput; ///< validated input, forward to DCS
    std::function<void(const std::string&)> log;
    SyncTracker* sync = nullptr;

    /// Ask every node to identify itself. v1 firmware ignores this frame; if
    /// no HELLO arrives within ~200 ms the caller falls back to the v1 ping.
    void start() {
        send(hn::ADDR_BROADCAST, hn::MSG_HELLO_REQUEST, nullptr, 0);
    }
    bool v2Confirmed() const { return v2_; }

    void feed(const uint8_t* data, size_t n) {
        for (size_t i = 0; i < n; i++)
            if (dec_.feed(data[i]) == hn::FrameDecoder::Result::Frame) handle();
    }

    void setMode(uint8_t mode) {
        mode_ = mode;
        send(hn::ADDR_BROADCAST, hn::MSG_MODE, &mode, 1);
        fullStateDue_ = true; // outputs need refreshing after lamp test etc.
    }
    uint8_t mode() const { return mode_; }

    void requestSync() { send(hn::ADDR_BROADCAST, hn::MSG_SYNC_REQUEST, nullptr, 0); }

    /// Broadcast the given ids (only those some node subscribed to).
    void sendState(const CatalogState& state, const std::vector<uint16_t>& ids) {
        std::vector<uint16_t> wanted;
        for (uint16_t id : ids)
            for (const auto& kv : nodes_)
                if (kv.second.online && kv.second.subscription.wants(id)) { wanted.push_back(id); break; }
        for (const auto& f : buildStateFrames(state, wanted, hn::ADDR_BROADCAST, seq_))
            if (write) write(f.data(), f.size());
    }

    /// True once after a node (re)joined or the mode changed: send full state now.
    bool takeFullStateDue() { const bool d = fullStateDue_; fullStateDue_ = false; return d; }

    const std::map<uint8_t, NodeInfo>& nodes() const { return nodes_; }
    const hn::LinkCounters& counters() const { return dec_.counters; }

private:
    void emitLog(const std::string& s) { if (log) log(s); }

    void send(uint8_t dst, uint8_t type, const uint8_t* p, size_t n) {
        hn::FrameHeader h;
        h.dst = dst;
        h.src = hn::ADDR_BRIDGE;
        h.type = type;
        h.seq = seq_++;
        uint8_t wire[hn::kMaxWireFrame];
        const size_t w = hn::encodeFrame(h, p, n, wire, sizeof(wire));
        if (w && write) write(wire, w);
    }

    NodeInfo& node(uint8_t a) {
        NodeInfo& n = nodes_[a];
        n.address = a;
        if (!n.online) { n.online = true; fullStateDue_ = true; }
        return n;
    }

    void handle() {
        const hn::FrameHeader& h = dec_.header();
        const uint8_t* p = dec_.payload();
        v2_ = true;
        emitLog(decodeFrame(h, p, h.len));
        switch (h.type) {
        case hn::MSG_HELLO: {
            NodeInfo& n = node(h.src);
            if (hn::readHello(p, h.len, n.hello)) {
                n.haveHello = true;
                const bool isMaster = (n.hello.role & hn::ROLE_BUS_MASTER) != 0;
                n.catalogMismatch = !isMaster && n.hello.catalogHash != Hornet::kCatalogHash;
                if (isMaster) n.subscription.scope = hn::SUBSCRIBE_CLEAR;
                if (n.catalogMismatch)
                    emitLog("WARNING: " + std::string(n.hello.name) + " (" + addressName(h.src) +
                            ") was built with a different control catalogue. Update the HornetLink library and re-upload.");
            }
            break;
        }
        case hn::MSG_SUBSCRIBE: {
            Subscription s;
            if (Subscription::parse(p, h.len, s)) { node(h.src).subscription = s; fullStateDue_ = true; }
            break;
        }
        case hn::MSG_INPUT: handleInput(h, p); break;
        case hn::MSG_SYNC_REPORT: {
            hn::SyncReader rd(p, h.len);
            hn::SyncRecord r;
            while (rd.next(r)) if (sync) sync->setCockpit(r.id, r.value);
            break;
        }
        case hn::MSG_BUS_EVENT:
            if (h.len >= 2) {
                if (p[0] == hn::BUS_JOIN) {
                    node(p[1]);
                    send(p[1], hn::MSG_HELLO_REQUEST, nullptr, 0);
                } else if (p[0] == hn::BUS_DROP) {
                    auto it = nodes_.find(p[1]);
                    if (it != nodes_.end()) it->second.online = false;
                    dedup_.forget(p[1]);
                }
            }
            break;
        default:
            break;
        }
    }

    void handleInput(const hn::FrameHeader& h, const uint8_t* p) {
        node(h.src);
        hn::InputReader rd(p, h.len);
        hn::InputRecord r;
        uint8_t nack = 0;
        uint16_t nackId = 0;
        while (rd.next(r)) {
            const uint8_t why = validateInput(r);
            if (why) { if (!nack) { nack = why; nackId = r.id; } continue; }
        }
        if (rd.error() && !nack) nack = hn::NACK_BAD_FRAME;
        if (!nack && mode_ != hn::MODE_SIM && mode_ != hn::MODE_WIRING_TEST) nack = hn::NACK_WRONG_MODE;
        if (nack) {
            uint8_t b[4] = {h.seq, nack, 0, 0};
            hn::putU16(b + 2, nackId);
            send(h.src, hn::MSG_NACK, b, 4);
        } else {
            const bool fresh = dedup_.accept(h.src, h.seq);
            if (fresh) {
                rd = hn::InputReader(p, h.len);
                while (rd.next(r)) {
                    if (sync && (r.action == hn::ACTION_SET_POSITION || r.action == hn::ACTION_ANALOG))
                        sync->setCockpit(r.id, r.arg);
                    if (mode_ == hn::MODE_WIRING_TEST)
                        emitLog("WIRING " + addressName(h.src) + " " + describeInput(r));
                    else if (onInput) onInput(h.src, r);
                }
            }
            send(h.src, hn::MSG_ACK, &h.seq, 1);
        }
    }

    hn::FrameDecoder dec_;
    hn::DuplicateFilter dedup_;
    std::map<uint8_t, NodeInfo> nodes_;
    uint8_t seq_ = 0;
    uint8_t mode_ = hn::MODE_SIM;
    bool v2_ = false;
    bool fullStateDue_ = false;
};

} // namespace hornet_native
