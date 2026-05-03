#pragma once
/**
 * @file ControlDatabase.hpp
 * @brief DCS-BIOS control-reference database and state-change logging helpers.
 *
 * @details
 * Loads the JSON control-reference files produced by the DCS-BIOS project and
 * exposes indexed lookup by byte address and by identifier string. Also
 * provides utilities for reading a control's current value from the live state
 * map and for deciding which controls are worth emitting in the operator log.
 *
 * ### JSON directory location (searched in order)
 * 1. `%USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\doc\json`
 * 2. Same path with `DCS.openbeta` and `DCS.openalpha` variants
 * 3. Paths relative to the bridge executable (for development installs)
 *
 * ### JSON schema (subset parsed by this implementation)
 * @code
 * {
 *   "CATEGORY_NAME": {
 *     "IDENTIFIER": {
 *       "description": "Human-readable description",
 *       "control_type": "selector|potentiometer|gauge|led|metadata|…",
 *       "outputs": [
 *         {
 *           "address": 0x1234,    // even byte address in the 64 KB state map
 *           "mask":    0x00FF,    // bit mask applied after reading the word
 *           "shift_by": 0,        // right-shift applied after masking
 *           "max_value": 255,     // maximum encoded value (0 for strings)
 *           "max_length": 10      // byte width of a string output (omitted for int)
 *         }
 *       ],
 *       "inputs": [
 *         { "interface": "set_state", … }  // presence means SET_STATE is accepted
 *       ]
 *     }
 *   }
 * }
 * @endcode
 *
 * @note load() is called once from the UI thread at startup. All lookup
 *       functions are read-only after that point and safe to call from the
 *       bridge worker thread without additional synchronisation.
 *
 * @copyright Copyright 2016-2026 Hornet Link contributors.
 *            Licensed under the Apache License, Version 2.0.
 */

#include "BiosProtocol.hpp"

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dcsbios {

/**
 * @brief In-memory index of DCS-BIOS control descriptors loaded from JSON.
 *
 * @details
 * Provides two lookup indexes built at load time:
 *   - By even byte address → list of descriptor pointers (multiple controls
 *     may share a word when they are bit-packed into the same address).
 *   - By identifier string → single descriptor.
 *
 * @thread_safety
 * load() must be called exactly once before any lookup. After that, all
 * const methods are safe for concurrent read access.
 */
class ControlDatabase {
public:
    /**
     * @brief Load all `*.json` files under @p jsonDir into the database.
     *
     * @param jsonDir       Absolute UTF-8 path to the directory containing JSON files.
     * @param moduleFilter  If non-empty, only load the file whose stem matches
     *                      this string (e.g. `"FA-18C_hornet"`). Pass an empty
     *                      string to load all files.
     * @return Number of unique controls successfully loaded.
     */
    size_t load(const std::string& jsonDir, const std::string& moduleFilter = {}) {
        byAddr_.clear();
        byId_.clear();

        try {
            namespace fs = std::filesystem;
            fs::path dir(jsonDir);
            if (!fs::is_directory(dir)) return 0;
            for (auto& entry : fs::directory_iterator(dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    if (!moduleFilter.empty() && entry.path().stem().string() != moduleFilter) {
                        continue;
                    }
                    loadFile(entry.path().string());
                }
            }
        } catch (...) {}

        return byId_.size();
    }

    /**
     * @brief Look up all controls whose output word is at @p byteAddr.
     *
     * Multiple controls may share an address when they occupy different bit
     * fields within the same 16-bit word.
     *
     * @param byteAddr  Even byte address to query.
     * @return Pointer to the list of descriptor pointers, or nullptr if no
     *         control is registered at that address.
     */
    const std::vector<const ControlDescriptor*>* lookupByAddr(uint16_t byteAddr) const {
        auto it = byAddr_.find(byteAddr);
        if (it == byAddr_.end()) return nullptr;
        return &it->second;
    }

    /**
     * @brief Look up a control by its DCS-BIOS identifier string.
     * @param id  Identifier to search for, e.g. `"UFC_OPTION1"`.
     * @return Pointer to the descriptor, or nullptr if not found.
     */
    const ControlDescriptor* lookupById(const std::string& id) const {
        auto it = byId_.find(id);
        if (it == byId_.end()) return nullptr;
        return &it->second;
    }

    /**
     * @brief Invoke @p callback for every control in the database.
     * @tparam Callback  Callable with signature `void(const ControlDescriptor&)`.
     */
    template <typename Callback>
    void forEachControl(Callback&& callback) const {
        for (const auto& entry : byId_) {
            callback(entry.second);
        }
    }

    /// @return True when no controls have been loaded.
    bool empty() const { return byId_.empty(); }

private:
    /// Address → list of descriptor pointers (multiple per address for bit-packed fields).
    std::unordered_map<uint16_t, std::vector<const ControlDescriptor*>> byAddr_;
    /// Identifier → owned descriptor (single canonical copy).
    std::unordered_map<std::string, ControlDescriptor> byId_;

    /**
     * @brief Parse one DCS-BIOS JSON control-reference file and insert its
     *        controls into the database.
     *
     * @details
     * The parser is intentionally minimal — it does not attempt full JSON
     * compliance. Instead it walks the known nesting structure:
     *
     *   `{ category: { identifier: { description, control_type, outputs: [...], inputs: [...] } } }`
     *
     * The depth counter tracks nested `{`/`[` tokens so the parser can
     * distinguish top-level control fields (depth == 1) from the same key
     * names appearing in nested output or input objects (depth > 1).
     *
     * @param path  Absolute UTF-8 path to a `*.json` file.
     */
    void loadFile(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) return;
        std::string json((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());

        // Determine module name from filename (strip directory and .json)
        std::string module;
        auto slash = path.find_last_of("\\/");
        auto dot   = path.rfind('.');
        if (slash != std::string::npos && dot != std::string::npos && dot > slash) {
            module = path.substr(slash + 1, dot - slash - 1);
        }

        // Walk through "identifier": { ... "outputs": [ { "address": N, "mask": N, "shift_by": N ... }] }
        // We do simple substring scanning — no full JSON parse needed.
        size_t pos = 0;

        // ── Local parser helpers ────────────────────────────────────────────
        // These lambdas capture `pos` and `json` by reference and advance the
        // cursor as they consume characters.

        /// Skip all whitespace characters at the current position.
        auto skipWs = [&]() { while (pos < json.size() && std::isspace((unsigned char)json[pos])) ++pos; };

        /// Consume and return a JSON string token starting with `"`.
        /// Returns an empty string if the current character is not `"`.
        auto readString = [&]() -> std::string {
            if (pos >= json.size() || json[pos] != '"') return {};
            ++pos;
            std::string s;
            while (pos < json.size() && json[pos] != '"') {
                if (json[pos] == '\\') { ++pos; }
                if (pos < json.size()) s += json[pos++];
            }
            if (pos < json.size()) ++pos; // closing "
            return s;
        };

        /// Consume and return a JSON integer (decimal or 0x-prefixed hex).
        /// Handles a leading `-` for negative values.
        auto readNumber = [&]() -> long long {
            skipWs();
            bool neg = false;
            if (pos < json.size() && json[pos] == '-') { neg = true; ++pos; }
            long long n = 0;
            while (pos < json.size() && std::isdigit((unsigned char)json[pos])) {
                n = n * 10 + (json[pos++] - '0');
            }
            // skip hex (0x...)
            if (pos + 1 < json.size() && json[pos] == 'x') {
                ++pos;
                n = 0;
                while (pos < json.size() && std::isxdigit((unsigned char)json[pos])) {
                    unsigned char c = (unsigned char)json[pos++];
                    n = n * 16 + (c >= 'a' ? c - 'a' + 10 : c >= 'A' ? c - 'A' + 10 : c - '0');
                }
            }
            return neg ? -n : n;
        };

        // Top level: object of categories
        skipWs(); if (pos >= json.size() || json[pos] != '{') return; ++pos;
        while (pos < json.size()) {
            skipWs();
            if (json[pos] == '}') break;
            if (json[pos] != '"') { ++pos; continue; }
            std::string category = readString();
            skipWs(); if (pos >= json.size() || json[pos] != ':') { ++pos; continue; } ++pos;
            skipWs(); if (pos >= json.size() || json[pos] != '{') { ++pos; continue; } ++pos;

            // controls in this category
            while (pos < json.size()) {
                skipWs();
                if (json[pos] == '}') { ++pos; break; }
                if (json[pos] != '"') { ++pos; continue; }
                std::string identifier = readString();
                skipWs(); if (pos >= json.size() || json[pos] != ':') { ++pos; continue; } ++pos;
                skipWs(); if (pos >= json.size() || json[pos] != '{') { ++pos; continue; } ++pos;

                // fields inside control object
                ControlDescriptor desc;
                desc.identifier = identifier;
                desc.category   = category;
                desc.module     = module;
                int depth = 1;
                std::string lastKey;

                while (pos < json.size() && depth > 0) {
                    skipWs();
                    char c = json[pos];
                    if (c == '{' || c == '[') { ++depth; ++pos; continue; }
                    if (c == '}' || c == ']') { --depth; ++pos; continue; }
                    if (c == ':') { ++pos; continue; }
                    if (c == ',') { ++pos; continue; }
                    if (c == '"') {
                        std::string token = readString();
                        skipWs();
                        if (pos < json.size() && json[pos] == ':') {
                            ++pos; lastKey = token;
                        } else {
                            // value string — only capture control-level description (depth 1),
                            // not output/input descriptions nested deeper
                            if (lastKey == "description" && depth == 1)
                                desc.description = token;
                            if (lastKey == "control_type" && depth == 1)
                                desc.controlType = token;
                            if (lastKey == "interface" && token == "set_state")
                                desc.hasSetStateInput = true;
                        }
                        continue;
                    }
                    if (std::isdigit((unsigned char)c) || c == '-') {
                        long long n = readNumber();
                        if (lastKey == "address")  desc.byteAddr = static_cast<uint16_t>(n);
                        else if (lastKey == "mask") desc.mask = static_cast<uint16_t>(n);
                        else if (lastKey == "shift_by") desc.shift = static_cast<uint8_t>(n);
                        else if (lastKey == "max_value") desc.maxVal = static_cast<uint32_t>(n);
                        else if (lastKey == "max_length") { desc.strLen = static_cast<uint16_t>(n); desc.isString = true; }
                        continue;
                    }
                    // booleans / null
                    ++pos;
                }

                if (!desc.identifier.empty() && (desc.mask != 0 || desc.isString)) {
                    auto [it, ok] = byId_.emplace(desc.identifier, desc);
                    if (ok) {
                        byAddr_[desc.byteAddr].push_back(&it->second);
                    }
                }

                skipWs(); if (pos < json.size() && json[pos] == ',') ++pos;
            }
            skipWs(); if (pos < json.size() && json[pos] == ',') ++pos;
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// State-change logging helpers
// ─────────────────────────────────────────────────────────────────────────────
// These free functions operate on the live BiosStateMap and are intentionally
// separate from ControlDatabase so that deduplication (comparison) and
// rendering (formatting) can evolve independently. The operator log mirrors
// the plain-text SET_STATE command format so log lines can be copy-pasted
// directly into a DCS-BIOS test script.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Read a control's current value from the state map.
 *
 * @details
 * For integer controls, returns the bit-field value extracted via
 * `(word & mask) >> shift`. For string controls, returns an FNV-1a hash of
 * the character bytes so the caller can detect string changes without storing
 * the full string in the baseline map.
 *
 * @param desc   Control descriptor (supplies address, mask, shift, isString).
 * @param state  The live DCS-BIOS state map.
 * @return A 32-bit value suitable for change detection via equality comparison.
 */
inline uint32_t ReadControlValue(const ControlDescriptor& desc, const BiosStateMap& state) {
    if (desc.isString) {
        uint32_t hash = 2166136261u;
        for (uint16_t i = 0; i < desc.strLen; ++i) {
            char ch = static_cast<char>(state.raw()[desc.byteAddr + i]);
            if (ch == 0) break;
            hash ^= static_cast<uint8_t>(ch);
            hash *= 16777619u;
        }
        return hash;
    }

    return state.readField(desc.byteAddr, desc.mask, desc.shift);
}

/**
 * @brief Format a control's current value as a DCS-BIOS `SET_STATE` log line.
 *
 * @details
 * Output format (wide string):
 * @code
 *   "IDENTIFIER SET_STATE value"    for integer controls
 *   "IDENTIFIER SET_STATE \"text\"" for string controls
 * @endcode
 *
 * @param desc   Control descriptor.
 * @param state  The live DCS-BIOS state map.
 * @return Wide string suitable for display in the operator log.
 */
inline std::wstring FormatWireStateChange(const ControlDescriptor& desc, const BiosStateMap& state) {
    auto toWide = [](const std::string& value) {
        return std::wstring(value.begin(), value.end());
    };

    wchar_t buf[512];
    if (desc.isString) {
        std::string s;
        for (uint16_t i = 0; i < desc.strLen; ++i) {
            char ch = static_cast<char>(state.raw()[desc.byteAddr + i]);
            if (ch == 0) break;
            s += ch;
        }

        std::wstring id = toWide(desc.identifier);
        std::wstring ws(s.begin(), s.end());
        swprintf_s(buf, L"%s SET_STATE \"%s\"", id.c_str(), ws.c_str());
    } else {
        uint32_t val = ReadControlValue(desc, state);
        std::wstring id = toWide(desc.identifier);
        swprintf_s(buf, L"%s SET_STATE %u", id.c_str(), static_cast<unsigned>(val));
    }
    return buf;
}

/**
 * @brief Decide whether a control's state transitions are worth logging.
 *
 * @details
 * The filtering rules follow the intent of the operator log — to show
 * actionable cockpit changes, not raw instrument readings or internal data:
 *
 * | Control category | Always logged | Conditional |
 * |------------------|---------------|-------------|
 * | String outputs   | No (too noisy)| — |
 * | Switch/selector (hasSetStateInput && maxVal ≤ 32) | Yes | — |
 * | Knob/dial (hasSetStateInput && maxVal > 32) | No | @p includeKnobDialRaw |
 * | Gauge (controlType == "gauge", no input) | No | @p includeGaugeRaw |
 * | Everything else | No | No |
 *
 * @param desc              Control descriptor.
 * @param includeKnobDialRaw  When true, also log high-range pilot-actuated controls.
 * @param includeGaugeRaw     When true, also log read-only gauge outputs.
 * @return True when this control's state changes should be emitted to the log.
 */
inline bool ShouldLogStateChange(const ControlDescriptor& desc,
                                 bool includeKnobDialRaw,
                                 bool includeGaugeRaw) {
    if (desc.isString) return false;

    // Pilot-actuated controls that accept SET_STATE.
    if (desc.hasSetStateInput) {
        if (desc.maxVal <= 32) return true;      // switches/selectors
        return includeKnobDialRaw;               // potentiometers/large-range dials
    }

    // Optional read-only gauge stream for debugging.
    if (!includeGaugeRaw) return false;

    std::string typeLower = desc.controlType;
    std::transform(typeLower.begin(), typeLower.end(), typeLower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return typeLower == "gauge";
}

} // namespace dcsbios
