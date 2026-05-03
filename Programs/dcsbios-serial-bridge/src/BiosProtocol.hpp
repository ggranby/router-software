#pragma once
/**
 * @file BiosProtocol.hpp
 * @brief DCS-BIOS export stream parser and in-memory state map.
 *
 * @details
 * Implements the two core data structures that bridge the DCS-BIOS network
 * stream to downstream hardware:
 *
 *  - **BiosStateMap** — flat 64 KB mirror of the DCS-BIOS address space with
 *    word-level dirty tracking.
 *  - **ExportParser** — byte-stream state machine that applies incoming write
 *    records to the state map and fires a callback after each complete frame.
 *
 * ### Export wire format (DCS → bridge, little-endian integers)
 * @code
 *   sync frame  : 0x55 0x55 0x55 0x55   (marks start of one update tick)
 *   write record: [addr_lo][addr_hi][len_lo][len_hi][data bytes…]
 * @endcode
 *
 * `addr` and `len` are always even. The sync sequence cannot appear in normal
 * data — DCS splits any coincident byte run if needed.
 *
 * ### Import wire format (device → DCS, plain-text UDP)
 * @code
 *   IDENTIFIER SET_STATE value\n
 *   IDENTIFIER TOGGLE\n
 * @endcode
 *
 * @copyright Copyright 2016-2026 Hornet Link contributors.
 *            Licensed under the Apache License, Version 2.0.
 */

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace dcsbios {

// ─────────────────────────────────────────────────────────────────────────────
// BiosStateMap
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Flat 64 KB mirror of the DCS-BIOS address space.
 *
 * @details
 * Maintains a byte-accurate copy of the DCS export address space and tracks
 * which 16-bit words changed since the last call to takeDirty(). Only even
 * byte addresses are tracked as dirty (each word spans two bytes, but the
 * DCS-BIOS protocol always writes to even-aligned addresses).
 *
 * @thread_safety
 * Not thread-safe. The owning ExportParser and the dispatch thread must
 * coordinate access externally (typically by consuming dirty addresses on the
 * same thread that calls processBytes / flushFrame).
 */
class BiosStateMap {
public:
    /// Total size of the DCS-BIOS address space in bytes (64 KB).
    static constexpr size_t kStateBytes = 65536;

    BiosStateMap() {
        state_.fill(0);
        dirty_.fill(false);
    }

    /**
     * @brief Apply one write record from the export stream to the state map.
     *
     * @param addr  Byte-aligned start address (always even per protocol).
     * @param data  Pointer to the incoming data bytes.
     * @param len   Number of bytes to write (always even per protocol).
     *
     * @details Only bytes that actually change are marked dirty, so the dirty
     * set reflects genuine state transitions rather than redundant writes.
     */
    void write(uint16_t addr, const uint8_t* data, uint16_t len) {
        for (uint16_t i = 0; i < len; ++i) {
            if (state_[addr + i] != data[i]) {
                state_[addr + i] = data[i];
                // mark entire containing 16-bit word dirty
                dirty_[(addr + i) & 0xFFFEu] = true;
            }
        }
    }

    /**
     * @brief Read a 16-bit word from the current state map.
     * @param byteAddr  Even byte address of the word (low byte).
     * @return The current 16-bit value at that address (little-endian).
     */
    uint16_t readWord(uint16_t byteAddr) const {
        uint16_t v = 0;
        v |= static_cast<uint16_t>(state_[byteAddr]);
        v |= static_cast<uint16_t>(state_[byteAddr + 1]) << 8;
        return v;
    }

    /**
     * @brief Extract a field value using a DCS-BIOS address/mask/shift descriptor.
     * @param byteAddr  Even byte address of the containing word.
     * @param mask      Bit mask applied after reading the word (e.g. 0x00FF).
     * @param shift     Right-shift amount applied after masking (in bits).
     * @return The extracted field value.
     */
    uint16_t readField(uint16_t byteAddr, uint16_t mask, uint8_t shift) const {
        return (readWord(byteAddr) & mask) >> shift;
    }

    /**
     * @brief Return all dirty word addresses and clear the dirty flags.
     *
     * @details Returns even byte addresses of every 16-bit word that changed
     * since the last call. Clears those flags atomically so subsequent calls
     * only return newly changed words.
     *
     * @return Sorted vector of even byte addresses with pending changes.
     */
    std::vector<uint16_t> takeDirty() {
        std::vector<uint16_t> result;
        for (size_t i = 0; i < kStateBytes; i += 2) {
            if (dirty_[i]) {
                result.push_back(static_cast<uint16_t>(i));
                dirty_[i] = false;
            }
        }
        return result;
    }

    const uint8_t* raw() const { return state_.data(); }

private:
    std::array<uint8_t, kStateBytes> state_;
    std::array<bool, kStateBytes>    dirty_;  // indexed by byte addr, only even indices used
};

// ─────────────────────────────────────────────────────────────────────────────
// ExportParser
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Byte-stream state machine that decodes the DCS-BIOS export protocol.
 *
 * @details
 * Feeds raw bytes (from UDP datagrams or a TCP stream) through the DCS-BIOS
 * export wire format and applies each complete write record to the associated
 * BiosStateMap. When a sync frame is detected (or when flushFrame() is called
 * for UDP datagrams), the @ref onFrameSync callback fires.
 *
 * ### Typical usage — UDP mode
 * @code
 *   ExportParser parser(stateMap);
 *   parser.onFrameSync = [&]() { dispatchDirtyAddresses(); };
 *   // For each received datagram:
 *   parser.processBytes(buf, len);
 *   parser.flushFrame();   // explicitly complete the frame for this datagram
 * @endcode
 *
 * ### Typical usage — TCP stream mode
 * @code
 *   parser.processBytes(buf, len);   // onFrameSync fires on each 0x55 sync
 * @endcode
 *
 * @thread_safety Not thread-safe. Call processBytes() and flushFrame() from
 *               the same thread that reads the dirty map.
 */
class ExportParser {
public:
    /**
     * @brief Construct a parser associated with the given state map.
     * @param map  The BiosStateMap that receives decoded write records.
     */
    explicit ExportParser(BiosStateMap& map) : map_(map) {}

    /**
     * @brief Callback invoked once per completed export frame.
     *
     * Fires after all write records for one DCS tick have been applied to the
     * state map. At call time the dirty flags are still set — the handler
     * should call BiosStateMap::takeDirty() to consume them.
     */
    std::function<void()> onFrameSync;

    /**
     * @brief Feed raw bytes from the network stream into the parser.
     * @param data  Pointer to the byte buffer.
     * @param len   Number of bytes to process.
     */
    void processBytes(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            processByte(data[i]);
        }
    }

    /**
     * @brief Explicitly complete the current frame (UDP mode only).
     *
     * @details
     * In UDP mode each datagram carries exactly one complete DCS-BIOS frame,
     * but there is no trailing sync sequence to trigger onFrameSync. Call this
     * after processBytes() returns for a datagram to fire the callback and
     * reset the parser for the next datagram.
     *
     * Has no effect in TCP mode (the sync header drives frame boundaries).
     */
    void flushFrame() {
        if (!inFrame_) return;
        if (state_ != State::AddrLo) return;
        if (onFrameSync) onFrameSync();
        inFrame_ = false;
        state_ = State::Sync0;
    }

private:
    /**
     * @brief Internal parser state machine states.
     *
     * The parser waits for four consecutive 0x55 bytes (Sync0–Sync3) before
     * entering the payload region. Each write record then provides an address
     * (AddrLo/AddrHi), a byte count (LenLo/LenHi), and that many data bytes.
     */
    enum class State { Sync0, Sync1, Sync2, Sync3, AddrLo, AddrHi, LenLo, LenHi, Data };

    BiosStateMap& map_;
    State         state_  = State::Sync0;
    uint16_t      addr_   = 0;
    uint16_t      len_    = 0;
    uint16_t      count_  = 0;
    uint8_t       buf_[4] = {};  // sync candidate buffer
    uint8_t       syncCnt_= 0;
    bool          inFrame_ = false;

    void processByte(uint8_t b) {
        switch (state_) {
        case State::Sync0:
            if (b == 0x55) { state_ = State::Sync1; } break;
        case State::Sync1:
            state_ = (b == 0x55) ? State::Sync2 : State::Sync0; break;
        case State::Sync2:
            state_ = (b == 0x55) ? State::Sync3 : State::Sync0; break;
        case State::Sync3:
            if (b == 0x55) {
                if (inFrame_ && onFrameSync) onFrameSync();
                inFrame_ = true;
                state_ = State::AddrLo;
            } else {
                state_ = State::Sync0;
            }
            break;
        case State::AddrLo:
            addr_ = b;
            state_ = State::AddrHi;
            break;
        case State::AddrHi:
            addr_ |= static_cast<uint16_t>(b) << 8;
            state_ = State::LenLo;
            break;
        case State::LenLo:
            len_ = b;
            state_ = State::LenHi;
            break;
        case State::LenHi:
            len_ |= static_cast<uint16_t>(b) << 8;
            count_ = 0;
            if (len_ == 0) { state_ = State::AddrLo; break; }
            state_ = State::Data;
            // Inline short records to avoid heap allocation
            dataBuf_.resize(len_);
            break;
        case State::Data:
            if (count_ < dataBuf_.size()) {
                dataBuf_[count_++] = b;
            }
            if (count_ == len_) {
                map_.write(addr_, dataBuf_.data(), len_);
                state_ = State::AddrLo;
            }
            break;
        }
    }

    std::vector<uint8_t> dataBuf_;
};

// ─────────────────────────────────────────────────────────────────────────────
// ControlDescriptor
// ─────────────────────────────────────────────────────────────────────────────
// ControlDescriptor
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Metadata for one addressable DCS-BIOS output control.
 *
 * @details
 * Loaded from the JSON control-reference directory at startup (typically from
 * the DCS-BIOS installation at
 * `%USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\doc\json`).
 *
 * Numeric controls occupy a contiguous bit field within a 16-bit word:
 * @code
 *   field_value = (word & mask) >> shift
 * @endcode
 *
 * String controls span consecutive bytes starting at @ref byteAddr with a
 * total width of @ref strLen bytes (null-terminated or space-padded).
 */
struct ControlDescriptor {
    std::string  identifier;        ///< DCS-BIOS identifier, e.g. `"UFC_OPTION1"`
    std::string  category;          ///< Panel category, e.g. `"UFC"`
    std::string  description;       ///< Human-readable description of the control
    std::string  controlType;       ///< Type tag: `selector`, `potentiometer`, `gauge`, `led`, `metadata`, …
    std::string  module;            ///< Aircraft module name, e.g. `"FA-18C_hornet"`
    uint16_t     byteAddr  = 0;     ///< Even byte address of the 16-bit word that contains this field
    uint16_t     mask      = 0xFFFF;///< Bit mask applied to the word before shifting
    uint8_t      shift     = 0;     ///< Right-shift applied after masking (in bits)
    uint32_t     maxVal    = 0;     ///< Maximum raw integer value (0 for unrestricted / string)
    bool         isString  = false; ///< True when the output is a character string, not an integer
    uint16_t     strLen    = 0;     ///< Byte width of a string output (0 for integer outputs)
    bool         hasSetStateInput = false; ///< True when DCS accepts a `SET_STATE` import command for this control
};

// ─────────────────────────────────────────────────────────────────────────────
// ImportCommand
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief One text command to forward to DCS-BIOS over UDP/TCP port 7778.
 *
 * @details
 * Commands follow the plain-text protocol accepted by DCS-BIOS:
 * @code
 *   IDENTIFIER ACTION\n               (e.g. "UFC_OPTION1 TOGGLE\n")
 *   IDENTIFIER ACTION VALUE\n         (e.g. "UFC_OPTION1 SET_STATE 3\n")
 * @endcode
 *
 * Produced by ImportLineParser when a device sends input events over its
 * bidirectional serial channel.
 */
struct ImportCommand {
    std::string identifier; ///< DCS-BIOS identifier of the target control
    std::string action;     ///< Command verb: `SET_STATE`, `TOGGLE`, `INC`, `DEC`, …
    std::string value;      ///< Argument for `SET_STATE`; empty for action-only commands

    /**
     * @brief Serialise the command to the newline-terminated wire format.
     * @return Ready-to-send string including trailing `\n`.
     */
    std::string toLine() const {
        if (value.empty()) {
            return identifier + " " + action + "\n";
        }
        return identifier + " " + action + " " + value + "\n";
    }
};

} // namespace dcsbios
