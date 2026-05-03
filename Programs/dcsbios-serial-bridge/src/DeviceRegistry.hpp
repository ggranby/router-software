#pragma once
/**
 * @file DeviceRegistry.hpp
 * @brief Per-device metadata, handshake protocol, delta-frame builder, and import-line parser.
 *
 * @details
 * This file owns the device-level framing layer that sits between the raw
 * serial bytes and the DCS-BIOS export protocol. It provides:
 *
 *  - **DeviceInfo** — runtime record for one connected COM port (role, name,
 *    subscriptions, bidirectional capability).
 *  - **HandshakeParser** — state machine that parses the 3-way probe/pong/ack
 *    sequence used to negotiate capabilities with Open-Hornet firmware.
 *  - **BuildDeltaFrame()** — builds a minimal DCS-BIOS wire frame containing
 *    only the dirty addresses that a given device subscribed to.
 *  - **ImportLineParser** — accumulates bytes into `\n`-terminated lines and
 *    fires a callback for each decoded `ImportCommand`.
 *
 * ### 3-way handshake overview
 * @code
 *   Bridge → Device:  AA DE AD 01                        (probe / ping)
 *   Device → Bridge:  AA DE AD 02  [flags] [name_len] [name…]
 *                                  [sub_count_lo] [sub_count_hi]
 *                                  [addr_lo addr_hi mask_lo mask_hi shift]…
 *   Bridge → Device:  AA DE AD 03                        (ack / negotiation complete)
 * @endcode
 *
 * **flags byte bits:**
 * | Bit | Meaning |
 * |-----|---------|
 * | 0x01 | Device is an RS-485 bus **master** (has downstream slaves) |
 * | 0x02 | Device is an RS-485 bus **slave** (managed by another master) |
 * | 0x04 | Device supports **bidirectional** operation (sends import commands) |
 *
 * Each subscription entry is 5 bytes: `addr_lo addr_hi mask_lo mask_hi shift`.
 * A single entry of `0xFFFF 0xFFFF 0xFFFF 0xFFFF 0x00` means "send everything".
 *
 * If the device does not reply within 300 ms it is treated as a **legacy**
 * dumb consumer and receives the full unfiltered byte stream (original behaviour).
 *
 * @copyright Copyright 2016-2026 Hornet Link contributors.
 *            Licensed under the Apache License, Version 2.0.
 */

#include "BiosProtocol.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace dcsbios {

// ─────────────────────────────────────────────────────────────────────────────
// Handshake frame constants
// ─────────────────────────────────────────────────────────────────────────────

/// @name Handshake frame header bytes
/// Three-byte magic prefix present in every probe frame direction.
///@{
constexpr uint8_t kProbeHdr0 = 0xAA; ///< Frame header byte 0
constexpr uint8_t kProbeHdr1 = 0xDE; ///< Frame header byte 1
constexpr uint8_t kProbeHdr2 = 0xAD; ///< Frame header byte 2
///@}

/// @name Handshake frame type bytes (fourth byte)
///@{
constexpr uint8_t kProbePing = 0x01; ///< Bridge → Device: initiate probe
constexpr uint8_t kProbePong = 0x02; ///< Device → Bridge: capability response
constexpr uint8_t kProbeAck  = 0x03; ///< Bridge → Device: negotiation complete
///@}

/// Maximum byte length of one import command line. Lines exceeding this limit
/// are silently discarded to prevent memory growth from framing errors.
constexpr size_t kImportLineMaxBytes = 256;

/// Maximum time the bridge waits for a pong response before classifying the
/// device as legacy (no handshake support).
constexpr auto kHandshakeTimeout = std::chrono::milliseconds(300);

// ─────────────────────────────────────────────────────────────────────────────
// Subscription
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief One DCS-BIOS output field that a device has subscribed to receive.
 *
 * @details
 * Mirrors the 5-byte subscription entry sent in the pong frame:
 * `[addr_lo addr_hi mask_lo mask_hi shift]`.
 *
 * A wildcard subscription (`byteAddr == 0xFFFF && mask == 0xFFFF`) means the
 * device wants to receive every dirty address — equivalent to the legacy
 * full-stream mode.
 */
struct Subscription {
    uint16_t byteAddr = 0xFFFF; ///< Even byte address of the 16-bit word containing the field
    uint16_t mask     = 0xFFFF; ///< Bit mask that isolates the field within the word
    uint8_t  shift    = 0;      ///< Right-shift applied after masking (in bits)

    /// @return True when this entry represents "subscribe to all addresses".
    bool wantsAll() const { return byteAddr == 0xFFFF && mask == 0xFFFF; }
};

// ─────────────────────────────────────────────────────────────────────────────
// SlaveInfo
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Describes one RS-485 slave declared by a master device during handshake.
 *
 * @details
 * When a device reports the RS-485 Master flag (0x01), it appends a slave list
 * to the pong frame after the subscription entries.  Each slave entry carries
 * the slave's bus address, a human-readable name, and its own subscription list
 * so the bridge can build per-slave delta frames.
 *
 * ### Slave-list wire layout (follows subscription entries for RS-485 masters)
 * @code
 *   slave_count : 1 byte          number of slave entries
 *   For each slave:
 *     slave_addr    : 1 byte      RS-485 bus address (1-255)
 *     slave_name_len: 1 byte      byte length of the slave name string
 *     slave_name    : name_len bytes
 *     slave_sub_count : 2 bytes   (little-endian) number of slave subscriptions
 *     For each slave subscription (5 bytes each):
 *       addr  : 2 bytes  (little-endian) even byte address
 *       mask  : 2 bytes  (little-endian) bit mask
 *       shift : 1 byte   right-shift amount
 * @endcode
 */
struct SlaveInfo {
    uint8_t  busAddress = 0;          ///< RS-485 bus address (1–255; 0 = unset)
    std::string name;                 ///< Human-readable slave name from firmware
    std::vector<Subscription> subs;   ///< Subscriptions declared by this slave
    std::unordered_set<uint16_t> subAddrs; ///< Fast-lookup set; built by buildAddrSet()
    bool wantsAll = true;             ///< True if any sub is a wildcard

    /// Populate subAddrs from subs (call once after all subs are loaded).
    void buildAddrSet() {
        wantsAll = false;
        subAddrs.clear();
        for (auto& s : subs) {
            if (s.wantsAll()) { wantsAll = true; subAddrs.clear(); return; }
            subAddrs.insert(s.byteAddr);
        }
    }

    /// @return True when this slave wants an update for @p byteAddr.
    bool wantsAddress(uint16_t byteAddr) const {
        if (wantsAll) return true;
        return subAddrs.count(byteAddr) > 0;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// DeviceRole
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Operational role of a connected device, established during handshake.
 */
enum class DeviceRole {
    Unknown,     ///< Not yet identified (handshake in progress or timed out)
    Legacy,      ///< No handshake reply — receives full unfiltered byte stream
    Standalone,  ///< Self-contained panel, no RS-485 bus downstream
    RS485Master, ///< Aggregates multiple slave panels on an RS-485 bus
    RS485Slave   ///< Managed by an RS-485 master; bridge talks to master only
};

// ─────────────────────────────────────────────────────────────────────────────
// DeviceInfo
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Runtime record for one device connected on a COM port.
 *
 * @details
 * Populated during the 3-way handshake and held for the lifetime of the
 * connection. @ref buildAddrSet() must be called once after all subscriptions
 * are parsed to construct the fast-lookup address set.
 */
struct DeviceInfo {
    std::string  comPort;             ///< Port identifier, e.g. `"COM10"`
    std::string  deviceName;          ///< Device name from handshake (empty for legacy)
    DeviceRole   role        = DeviceRole::Unknown; ///< Operational role (set by handshake)
    bool         bidir       = false;   ///< True when the device can send import commands
    bool         handshakeDone = false; ///< True once handshake succeeded or timed out

    std::vector<Subscription> subscriptions; ///< Ordered subscription list from pong frame
    std::vector<SlaveInfo>    slaves;         ///< Slave declarations (RS-485 masters only)

    /// Set of subscribed word addresses for O(1) dispatch filtering.
    /// Populated by buildAddrSet(); empty when wantsAll is true.
    std::unordered_set<uint16_t> subAddrs;

    /// True until handshake establishes explicit subscriptions.
    /// When true the device receives every dirty address.
    bool wantsAll = true;

    /**
     * @brief Build the fast-lookup address set from the subscriptions list.
     *
     * @details
     * Must be called once after all subscriptions have been added (i.e. after
     * HandshakeParser::populateDevice()). If any subscription is a wildcard
     * the set is cleared and wantsAll is set to true.
     */
    void buildAddrSet() {
        wantsAll = false;
        subAddrs.clear();
        for (auto& s : subscriptions) {
            if (s.wantsAll()) { wantsAll = true; subAddrs.clear(); return; }
            subAddrs.insert(s.byteAddr);
        }
    }

    /**
     * @brief Return true when this device should receive an update for the given address.
     * @param byteAddr  Even byte address to test.
     */
    bool wantsAddress(uint16_t byteAddr) const {
        if (wantsAll) return true;
        return subAddrs.count(byteAddr) > 0;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// HandshakeParser
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief State machine that parses the device pong frame.
 *
 * @details
 * Feed bytes from the COM port one at a time via processByte(). The parser
 * advances through the pong frame fields (header, flags, name, subscription
 * list) and returns Complete when the entire frame has been parsed successfully.
 *
 * On completion, call populateDevice() to transfer the parsed fields into a
 * DeviceInfo record.
 *
 * ### Pong frame wire layout
 * @code
 *   AA DE AD 02          header + type byte
 *   flags : 1 byte       capability flags (RS-485 role + bidir)
 *   name_len : 1 byte    byte length of the device name string
 *   name : name_len bytes
 *   sub_count : 2 bytes  (little-endian) number of subscription entries
 *   For each subscription entry (5 bytes each):
 *     addr : 2 bytes     (little-endian) even byte address
 *     mask : 2 bytes     (little-endian) bit mask
 *     shift : 1 byte     right-shift amount
 * @endcode
 */
class HandshakeParser {
public:
    /// Result returned by processByte() after each call.
    enum class Result { Pending, Complete, Failed };

    HandshakeParser() = default;

    /**
     * @brief Feed one byte from the COM port into the parser.
     * @param b  The incoming byte.
     * @return   Pending while the frame is incomplete; Complete or Failed when done.
     */
    Result processByte(uint8_t b) {
        if (state_ == ParseState::Complete) return Result::Complete;
        switch (state_) {

        // ── Header (magic bytes + frame type) ────────────────────────────
        case ParseState::Header0:
            state_ = (b == kProbeHdr0) ? ParseState::Header1 : ParseState::Header0;
            break;
        case ParseState::Header1:
            state_ = (b == kProbeHdr1) ? ParseState::Header2 : ParseState::Header0;
            break;
        case ParseState::Header2:
            state_ = (b == kProbeHdr2) ? ParseState::FrameType : ParseState::Header0;
            break;
        case ParseState::FrameType:
            if (b != kProbePong) return Result::Failed;
            state_ = ParseState::Flags;
            break;

        // ── Capability flags ──────────────────────────────────────────────
        case ParseState::Flags:
            flags_ = b;
            state_ = ParseState::NameLen;
            break;

        // ── Device name (length-prefixed) ─────────────────────────────────
        case ParseState::NameLen:
            nameLen_  = b;
            nameRead_ = 0;
            // Skip the name data state if there is nothing to read.
            state_ = (nameLen_ > 0) ? ParseState::NameData : ParseState::SubCountLo;
            break;
        case ParseState::NameData:
            name_ += static_cast<char>(b);
            if (++nameRead_ == nameLen_)
                state_ = ParseState::SubCountLo;
            break;

        // ── Subscription count (little-endian 16-bit) ─────────────────────
        case ParseState::SubCountLo:
            subCount_ = b;
            state_ = ParseState::SubCountHi;
            break;
        case ParseState::SubCountHi:
            subCount_ |= static_cast<uint16_t>(b) << 8;
            state_ = (subCount_ == 0) ? ParseState::Complete : ParseState::SubAddrLo;
            break;

        // ── Subscription entry fields ─────────────────────────────────────
        case ParseState::SubAddrLo:
            curSub_.byteAddr = b;
            state_ = ParseState::SubAddrHi;
            break;
        case ParseState::SubAddrHi:
            curSub_.byteAddr |= static_cast<uint16_t>(b) << 8;
            state_ = ParseState::SubMaskLo;
            break;
        case ParseState::SubMaskLo:
            curSub_.mask = b;
            state_ = ParseState::SubMaskHi;
            break;
        case ParseState::SubMaskHi:
            curSub_.mask |= static_cast<uint16_t>(b) << 8;
            curSub_.shift = 0;
            state_ = ParseState::SubShift;
            break;
        case ParseState::SubShift:
            curSub_.shift = b;
            subs_.push_back(curSub_);
            if (static_cast<int>(subs_.size()) == subCount_) {
                // All master subscriptions read.  If the device is an RS-485
                // master (flags_ & 0x01) a slave list follows; otherwise done.
                state_ = (flags_ & 0x01) ? ParseState::SlaveCount : ParseState::Complete;
            } else {
                state_ = ParseState::SubAddrLo;
            }
            break;

        // ── RS-485 slave list (master devices only) ───────────────────────
        case ParseState::SlaveCount:
            slaveCount_ = b;
            slaveRead_  = 0;
            state_ = (slaveCount_ == 0) ? ParseState::Complete : ParseState::SlaveAddr;
            break;
        case ParseState::SlaveAddr:
            curSlave_ = {};
            curSlave_.busAddress = b;
            state_ = ParseState::SlaveNameLen;
            break;
        case ParseState::SlaveNameLen:
            slaveNameLen_  = b;
            slaveNameRead_ = 0;
            state_ = (slaveNameLen_ > 0) ? ParseState::SlaveNameData : ParseState::SlaveSubCountLo;
            break;
        case ParseState::SlaveNameData:
            curSlave_.name += static_cast<char>(b);
            if (++slaveNameRead_ == slaveNameLen_)
                state_ = ParseState::SlaveSubCountLo;
            break;
        case ParseState::SlaveSubCountLo:
            slaveSubCount_ = b;
            state_ = ParseState::SlaveSubCountHi;
            break;
        case ParseState::SlaveSubCountHi:
            slaveSubCount_ |= static_cast<uint16_t>(b) << 8;
            state_ = (slaveSubCount_ == 0) ? ParseState::SlaveFinish : ParseState::SlaveSubAddrLo;
            break;
        case ParseState::SlaveSubAddrLo:
            curSlaveSub_.byteAddr = b;
            state_ = ParseState::SlaveSubAddrHi;
            break;
        case ParseState::SlaveSubAddrHi:
            curSlaveSub_.byteAddr |= static_cast<uint16_t>(b) << 8;
            state_ = ParseState::SlaveSubMaskLo;
            break;
        case ParseState::SlaveSubMaskLo:
            curSlaveSub_.mask = b;
            state_ = ParseState::SlaveSubMaskHi;
            break;
        case ParseState::SlaveSubMaskHi:
            curSlaveSub_.mask |= static_cast<uint16_t>(b) << 8;
            state_ = ParseState::SlaveSubShift;
            break;
        case ParseState::SlaveSubShift:
            curSlaveSub_.shift = b;
            curSlave_.subs.push_back(curSlaveSub_);
            if (static_cast<int>(curSlave_.subs.size()) == slaveSubCount_) {
                // All subs for this slave parsed.  Store slave, check for more.
                curSlave_.buildAddrSet();
                slaves_.push_back(std::move(curSlave_));
                if (++slaveRead_ == slaveCount_)
                    state_ = ParseState::Complete;
                else
                    state_ = ParseState::SlaveAddr;
            } else {
                state_ = ParseState::SlaveSubAddrLo;
            }
            break;
        case ParseState::SlaveFinish:
            // Entered when a slave has sub_count == 0 (no subs declared yet).
            // In that case SlaveSubCountHi transitioned to SlaveFinish,
            // consuming the byte there.  Here we store and advance.
            curSlave_.buildAddrSet();
            slaves_.push_back(std::move(curSlave_));
            if (++slaveRead_ == slaveCount_)
                state_ = ParseState::Complete;
            else
                state_ = ParseState::SlaveAddr;
            break;

        case ParseState::Complete:
            return Result::Complete;
        }

        return (state_ == ParseState::Complete) ? Result::Complete : Result::Pending;
    }

    /// @return True when a complete pong frame has been successfully parsed.
    bool complete() const { return state_ == ParseState::Complete; }

    /**
     * @brief Populate a DeviceInfo record from the successfully parsed pong frame.
     * @param d  Output record to fill. Must be called only after complete() is true.
     */
    void populateDevice(DeviceInfo& d) const {
        d.deviceName  = name_;
        d.bidir       = (flags_ & 0x04) != 0;
        d.subscriptions = subs_;
        d.slaves        = slaves_;
        if      (flags_ & 0x01) d.role = DeviceRole::RS485Master;
        else if (flags_ & 0x02) d.role = DeviceRole::RS485Slave;
        else                    d.role = DeviceRole::Standalone;
        d.handshakeDone = true;
        d.buildAddrSet();
    }

    /**
     * @brief Build the 4-byte probe (ping) frame the bridge sends to initiate the handshake.
     * @return Byte array ready to pass to WriteFile().
     */
    static std::array<uint8_t, 4> pingFrame() {
        return { kProbeHdr0, kProbeHdr1, kProbeHdr2, kProbePing };
    }

    /**
     * @brief Build the 4-byte acknowledgement frame sent after a successful handshake.
     * @return Byte array ready to pass to WriteFile().
     */
    static std::array<uint8_t, 4> ackFrame() {
        return { kProbeHdr0, kProbeHdr1, kProbeHdr2, kProbeAck };
    }

private:
    /**
     * @brief Parser state enumeration — one value per field in the pong frame.
     *
     * States map directly to the wire-format layout documented in the class
     * description above. The parser advances linearly; there is no backtracking
     * except to Header0 when a header byte is invalid.
     */
    enum class ParseState {
        Header0,         ///< Waiting for first header byte (0xAA)
        Header1,         ///< Waiting for second header byte (0xDE)
        Header2,         ///< Waiting for third header byte (0xAD)
        FrameType,       ///< Waiting for frame type byte (must be kProbePong = 0x02)
        Flags,           ///< Reading capability flags byte
        NameLen,         ///< Reading device name length byte
        NameData,        ///< Reading name characters (nameLen_ bytes total)
        SubCountLo,      ///< Reading low byte of subscription count
        SubCountHi,      ///< Reading high byte of subscription count
        SubAddrLo,       ///< Reading low byte of current subscription address
        SubAddrHi,       ///< Reading high byte of current subscription address
        SubMaskLo,       ///< Reading low byte of current subscription mask
        SubMaskHi,       ///< Reading high byte of current subscription mask
        SubShift,        ///< Reading shift byte of current subscription
        // RS-485 slave list (RS485Master devices only)
        SlaveCount,      ///< Reading slave count byte
        SlaveAddr,       ///< Reading slave bus address byte
        SlaveNameLen,    ///< Reading slave name length byte
        SlaveNameData,   ///< Reading slave name characters
        SlaveSubCountLo, ///< Reading low byte of slave subscription count
        SlaveSubCountHi, ///< Reading high byte of slave subscription count
        SlaveSubAddrLo,  ///< Reading low byte of current slave subscription address
        SlaveSubAddrHi,  ///< Reading high byte of current slave subscription address
        SlaveSubMaskLo,  ///< Reading low byte of current slave subscription mask
        SlaveSubMaskHi,  ///< Reading high byte of current slave subscription mask
        SlaveSubShift,   ///< Reading shift byte of current slave subscription
        SlaveFinish,     ///< Current slave complete — store and decide next action
        Complete,        ///< Entire pong frame consumed successfully
    };

    ParseState state_    = ParseState::Header0;
    uint8_t    flags_    = 0;
    uint8_t    nameLen_  = 0;
    std::string name_;
    uint8_t    nameRead_ = 0;
    uint16_t   subCount_ = 0;
    std::vector<Subscription> subs_;
    Subscription curSub_;
    // Slave list parser state
    uint8_t    slaveCount_   = 0;
    uint8_t    slaveRead_    = 0;
    uint8_t    slaveNameLen_ = 0;
    uint8_t    slaveNameRead_= 0;
    uint16_t   slaveSubCount_= 0;
    SlaveInfo  curSlave_;
    Subscription curSlaveSub_;
    std::vector<SlaveInfo> slaves_;
};

// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
// BuildDeltaFrame
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Build a DCS-BIOS wire frame containing only the dirty addresses a device subscribed to.
 *
 * @details
 * Filters @p dirtyAddrs against the device's subscription set, then packs the
 * relevant words into the fewest possible DCS-BIOS write records by merging
 * consecutive even addresses into a single run.
 *
 * Output format:
 * @code
 *   [sync: 0x55 0x55 0x55 0x55]
 *   [addr_lo][addr_hi][len_lo][len_hi][data…]  (one entry per consecutive run)
 * @endcode
 *
 * @param state       The current DCS-BIOS state map (source of word values).
 * @param dirtyAddrs  Sorted list of even byte addresses with pending changes.
 * @param dev         Device record whose subscription determines which addresses to include.
 * @return            Ready-to-send byte buffer, or an empty vector if the device
 *                    has no interest in any of the dirty addresses.
 */
inline std::vector<uint8_t> BuildDeltaFrame(
    const BiosStateMap&           state,
    const std::vector<uint16_t>&  dirtyAddrs,
    const DeviceInfo&             dev)
{
    // Collect addresses this device cares about, sorted.
    std::vector<uint16_t> relevant;
    relevant.reserve(dirtyAddrs.size());
    for (uint16_t a : dirtyAddrs) {
        if (dev.wantsAddress(a)) relevant.push_back(a);
    }
    if (relevant.empty()) return {};
    std::sort(relevant.begin(), relevant.end());

    // Build DCS-BIOS wire format:
    // [sync: 0x55 0x55 0x55 0x55]
    // [addr_lo addr_hi len_lo len_hi data…] per run of consecutive addresses
    std::vector<uint8_t> frame;
    frame.reserve(4 + relevant.size() * 6);
    frame.push_back(0x55); frame.push_back(0x55); frame.push_back(0x55); frame.push_back(0x55);

    size_t i = 0;
    while (i < relevant.size()) {
        // Find consecutive run
        size_t j = i;
        while (j + 1 < relevant.size() && relevant[j + 1] == relevant[j] + 2) ++j;
        uint16_t runStart = relevant[i];
        uint16_t runWords = static_cast<uint16_t>(j - i + 1);
        uint16_t lenBytes = runWords * 2;

        frame.push_back(runStart & 0xFF);
        frame.push_back((runStart >> 8) & 0xFF);
        frame.push_back(lenBytes & 0xFF);
        frame.push_back((lenBytes >> 8) & 0xFF);
        for (uint16_t w = 0; w < runWords; ++w) {
            uint16_t a = runStart + w * 2;
            frame.push_back(state.raw()[a]);
            frame.push_back(state.raw()[a + 1]);
        }
        i = j + 1;
    }
    return frame;
}

// ─────────────────────────────────────────────────────────────────────────────
// ImportLineParser
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Accumulates raw bytes from a bidirectional device and fires a callback
 *        for each complete `\n`-terminated import command line.
 *
 * @details
 * Devices that declare the bidirectional flag in their handshake can send
 * DCS-BIOS import commands back to the bridge over the same serial connection.
 * This parser reassembles the byte stream into newline-terminated lines and
 * decodes each line as an ImportCommand.
 *
 * Expected line format:
 * @code
 *   IDENTIFIER ACTION\n
 *   IDENTIFIER ACTION VALUE\n
 * @endcode
 *
 * Lines longer than 256 bytes are silently discarded to prevent unbounded
 * buffer growth in the event of framing errors.
 *
 * @thread_safety Not thread-safe. Must be called from the ReadDeviceThread
 *               that owns this serial port.
 */
class ImportLineParser {
public:
    /// Callback invoked for each fully parsed import command line.
    std::function<void(const ImportCommand&)> onCommand;

    /**
     * @brief Feed raw bytes from the device serial port into the parser.
     * @param data  Pointer to the incoming byte buffer.
     * @param len   Number of bytes to process.
     */
    void processBytes(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            char c = static_cast<char>(data[i]);
            if (c == '\n' || c == '\r') {
                if (!line_.empty()) parseLine();
                line_.clear();
            } else if (line_.size() < kImportLineMaxBytes) {
                line_ += c;
            }
        }
    }

private:
    std::string line_;

    /// Parse the accumulated line into an ImportCommand and fire onCommand.
    void parseLine() {
        // Expected format: IDENTIFIER ACTION [VALUE]
        auto sp1 = line_.find(' ');
        if (sp1 == std::string::npos) return;
        ImportCommand cmd;
        cmd.identifier = line_.substr(0, sp1);
        auto rest = line_.substr(sp1 + 1);
        auto sp2 = rest.find(' ');
        if (sp2 == std::string::npos) {
            cmd.action = rest;
        } else {
            cmd.action = rest.substr(0, sp2);
            cmd.value  = rest.substr(sp2 + 1);
        }
        if (!cmd.identifier.empty() && !cmd.action.empty() && onCommand) {
            onCommand(cmd);
        }
    }
};

} // namespace dcsbios
