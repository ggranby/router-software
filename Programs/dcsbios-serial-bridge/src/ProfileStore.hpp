#pragma once
/**
 * @file ProfileStore.hpp
 * @brief Persistent device profile store — auto-applies panel templates by device name.
 *
 * @details
 * When a device responds to the handshake with a name (e.g.
 * `"1A1-UIP_ABSIS_BUS_MASTER"`), ProfileStore looks it up in two sources:
 *
 *  1. **`device_profiles.json`** — persistent per-operator overrides stored
 *     beside the executable.  Takes precedence over built-in templates.
 *  2. **`templates/panels.json`** — built-in panel templates shipped with
 *     Hornet Link that map device names to subscription address ranges.
 *
 * ### File format — `device_profiles.json`
 * @code
 * {
 *   "1A1-UIP_ABSIS_BUS_MASTER": {
 *     "templateName": "UIP_ABSIS",
 *     "subscriptions": [5128, 5130, 5132],
 *     "wantsAll": false
 *   }
 * }
 * @endcode
 *
 * ### File format — `templates/panels.json`
 * @code
 * {
 *   "UIP_ABSIS": {
 *     "description": "Upper Instrument Panel ABSIS board",
 *     "subscriptions": [5128, 5130, 5132, 5134]
 *   },
 *   "MASTER_ARM": {
 *     "description": "Master Arm Panel",
 *     "subscriptions": [13312, 13314]
 *   }
 * }
 * @endcode
 *
 * ### OpenHornet naming convention
 * Device names follow the OpenHornet reference designator format:
 * `{panel_num}{letter}{board_num}-{DESCRIPTION}`.  For example:
 * - `1A1-UIP_ABSIS_BUS_MASTER` — Upper IP, ABSIS board, RS485 master
 * - `1A2-MASTER_ARM_PANEL` — Upper IP, second board, standalone
 *
 * The `{X}A1` board in each panel group is always the RS485 bus master;
 * `{X}A2`, `{X}A3`, etc. are RS485 slaves.
 *
 * ### Thread safety
 * ProfileStore is not thread-safe.  All access must be from the UI thread
 * (or the BridgeController worker thread, which is the only caller during
 * a session — just not both simultaneously).
 *
 * @copyright Copyright 2016-2026 Hornet Link contributors.
 *            Licensed under the Apache License, Version 2.0.
 */

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace dcsbios {

/// Upper bound on profile/template JSON file size; larger files are rejected.
inline constexpr std::uintmax_t kProfileFileMaxBytes = 1024 * 1024;

/// Maximum accepted device-name length (bytes) for persisted profiles.
inline constexpr size_t kDeviceNameMaxBytes = 255;

/**
 * @brief True if @p name is acceptable as a persisted device-name key.
 *
 * Device names come from the handshake and are therefore untrusted. Only
 * printable ASCII up to kDeviceNameMaxBytes is accepted.
 */
inline bool IsValidDeviceName(const std::string& name) {
    if (name.empty() || name.size() > kDeviceNameMaxBytes) return false;
    return std::all_of(name.begin(), name.end(), [](char c) {
        return c >= 0x20 && c <= 0x7E;
    });
}

/**
 * @brief Escape @p value for inclusion inside a JSON string literal.
 */
inline std::string JsonEscape(const std::string& value) {
    static const char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(value.size() + 2);
    for (unsigned char c : value) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if (c < 0x20) {
                out += "\\u00";
                out += kHex[c >> 4];
                out += kHex[c & 0xF];
            } else {
                out += static_cast<char>(c);
            }
        }
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// DeviceProfile
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Resolved configuration for one named device.
 *
 * Populated by ProfileStore::resolve() from the stored profile or a template
 * match.  Applied to the DeviceInfo after the handshake completes.
 */
struct DeviceProfile {
    std::string              templateName;   ///< Source template name (or "custom")
    std::vector<uint16_t>    subscriptions;  ///< Address words this device wants to receive
    bool                     wantsAll = false; ///< True → bypass subscription filter
    bool                     fromUserProfile = false; ///< True → loaded from device_profiles.json
};

// ─────────────────────────────────────────────────────────────────────────────
// ProfileStore
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Loads, caches, and persists device profiles.
 *
 * Call `load()` once at startup to populate from disk.  Call `resolve()` at
 * handshake time to get the profile for a named device.  Call `save()` when
 * a user assigns a new template to a device via the UI.
 */
class ProfileStore {
public:
    ProfileStore() = default;

    /**
     * @brief Load profiles and templates from disk.
     *
     * @param exeDir  Directory containing the executable.  Used to locate:
     *                - `device_profiles.json` (beside the exe)
     *                - `templates/panels.json` (in a subdirectory)
     *
     * @return Number of user profiles + templates loaded (for log messages).
     */
    size_t load(const std::filesystem::path& exeDir) {
        exeDir_ = exeDir;
        size_t total = 0;
        total += loadUserProfiles(exeDir / "device_profiles.json");
        total += loadTemplates(exeDir / "templates" / "panels.json");
        return total;
    }

    /**
     * @brief Return the profile for a device by name, or nullopt if unknown.
     *
     * @details
     * Lookup order:
     *  1. User profiles (`device_profiles.json`) — exact name match.
     *  2. Template by name match — matches name suffix after the last `-`.
     *     E.g. `"1A1-UIP_ABSIS_BUS_MASTER"` → template `"UIP_ABSIS_BUS_MASTER"`.
     *  3. No match → returns nullopt; caller should prompt the user.
     *
     * @param deviceName  Device name from the handshake response field.
     * @return Resolved profile, or nullopt if no match found.
     */
    std::optional<DeviceProfile> resolve(const std::string& deviceName) const {
        // 1. Exact user-profile match
        {
            auto it = userProfiles_.find(deviceName);
            if (it != userProfiles_.end()) {
                DeviceProfile p = it->second;
                p.fromUserProfile = true;
                return p;
            }
        }

        // 2. Template match on suffix (after last '-')
        {
            std::string suffix = deviceName;
            auto pos = deviceName.rfind('-');
            if (pos != std::string::npos) suffix = deviceName.substr(pos + 1);

            auto it = templates_.find(suffix);
            if (it != templates_.end()) {
                DeviceProfile p;
                p.templateName  = it->first;
                p.subscriptions = it->second.subscriptions;
                p.wantsAll      = false;
                p.fromUserProfile = false;
                return p;
            }
        }

        return std::nullopt;
    }

    /**
     * @brief Persist a profile assignment to `device_profiles.json`.
     *
     * Called when the user assigns or re-assigns a panel template to a device
     * via the UI.  The file is written to a temporary file and then renamed
     * over the original, so a failed write never truncates existing profiles.
     *
     * @param deviceName  Device name key (from handshake).
     * @param profile     Profile to store.
     * @return True if the profile was accepted and written to disk; false if
     *         the name is invalid or the file could not be written.
     */
    bool save(const std::string& deviceName, const DeviceProfile& profile) {
        if (!IsValidDeviceName(deviceName)) {
            lastError_ = "invalid device name";
            return false;
        }
        userProfiles_[deviceName] = profile;
        return flushUserProfiles();
    }

    /**
     * @brief Return all known template names for UI display.
     */
    std::vector<std::string> templateNames() const {
        std::vector<std::string> names;
        names.reserve(templates_.size());
        for (const auto& kv : templates_) names.push_back(kv.first);
        std::sort(names.begin(), names.end());
        return names;
    }

    /**
     * @brief Return subscriptions for a named template, or empty if not found.
     */
    std::optional<std::vector<uint16_t>> templateSubscriptions(const std::string& name) const {
        auto it = templates_.find(name);
        if (it == templates_.end()) return std::nullopt;
        return it->second.subscriptions;
    }

    /**
     * @brief Description of the most recent load/save failure (empty if none).
     */
    const std::string& lastError() const { return lastError_; }

private:
    // ── Internal types ────────────────────────────────────────────────────────

    struct TemplateEntry {
        std::string           description;
        std::vector<uint16_t> subscriptions;
    };

    // ── Minimal JSON reader ───────────────────────────────────────────────────
    // Hornet Link has no JSON library dependency, so this is a small strict
    // reader for the two-level `{ "name": { "key": value, ... } }` files used
    // here. Any syntax error aborts the load; nothing partial is kept.

    class Reader {
    public:
        explicit Reader(const std::string& s) : s_(s) {}

        bool ok() const { return ok_; }
        void fail() { ok_ = false; }

        void ws() {
            while (pos_ < s_.size() &&
                   (s_[pos_] == ' ' || s_[pos_] == '\t' || s_[pos_] == '\r' || s_[pos_] == '\n'))
                ++pos_;
        }

        bool peek(char c) { ws(); return ok_ && pos_ < s_.size() && s_[pos_] == c; }

        bool consume(char c) {
            if (peek(c)) { ++pos_; return true; }
            return false;
        }

        void expect(char c) { if (!consume(c)) fail(); }

        bool atEnd() { ws(); return pos_ >= s_.size(); }

        std::string string() {
            std::string out;
            if (!consume('"')) { fail(); return out; }
            while (ok_) {
                if (pos_ >= s_.size()) { fail(); break; }
                char c = s_[pos_++];
                if (c == '"') return out;
                if (static_cast<unsigned char>(c) < 0x20) { fail(); break; }
                if (c != '\\') { out += c; continue; }
                if (pos_ >= s_.size()) { fail(); break; }
                char e = s_[pos_++];
                switch (e) {
                case '"': case '\\': case '/': out += e; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    uint32_t codePoint = 0;
                    if (!readHexCodeUnit(codePoint)) { fail(); break; }
                    if (codePoint >= 0xD800 && codePoint <= 0xDBFF) {
                        if (pos_ + 2 > s_.size() || s_[pos_] != '\\' || s_[pos_ + 1] != 'u') {
                            fail();
                            break;
                        }
                        pos_ += 2;
                        uint32_t low = 0;
                        if (!readHexCodeUnit(low) || low < 0xDC00 || low > 0xDFFF) {
                            fail();
                            break;
                        }
                        codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (low - 0xDC00);
                    } else if (codePoint >= 0xDC00 && codePoint <= 0xDFFF) {
                        fail();
                        break;
                    }
                    appendUtf8(out, codePoint);
                    break;
                }
                default: fail(); break;
                }
            }
            return out;
        }

        /// Parse a non-negative integer in [0, maxValue].
        std::optional<uint32_t> unsignedInt(uint32_t maxValue) {
            ws();
            size_t start = pos_;
            uint64_t n = 0;
            while (pos_ < s_.size() && s_[pos_] >= '0' && s_[pos_] <= '9') {
                n = n * 10 + static_cast<uint64_t>(s_[pos_] - '0');
                if (n > maxValue) { fail(); return std::nullopt; }
                ++pos_;
            }
            if (pos_ == start) { fail(); return std::nullopt; }
            return static_cast<uint32_t>(n);
        }

        std::optional<bool> boolean() {
            ws();
            if (s_.compare(pos_, 4, "true") == 0)  { pos_ += 4; return true; }
            if (s_.compare(pos_, 5, "false") == 0) { pos_ += 5; return false; }
            fail();
            return std::nullopt;
        }

        std::vector<uint16_t> u16Array() {
            std::vector<uint16_t> out;
            expect('[');
            if (consume(']')) return out;
            while (ok_) {
                auto v = unsignedInt(0xFFFF);
                if (!v) break;
                out.push_back(static_cast<uint16_t>(*v));
                if (consume(',')) continue;
                expect(']');
                break;
            }
            return out;
        }

        /// Skip any JSON value (used for unknown keys, for forward compatibility).
        void skipValue(int depth = 0) {
            if (depth > 16) { fail(); return; }
            ws();
            if (pos_ >= s_.size()) { fail(); return; }
            char c = s_[pos_];
            if (c == '"') { string(); return; }
            if (c == '{' || c == '[') {
                char close = (c == '{') ? '}' : ']';
                ++pos_;
                if (consume(close)) return;
                while (ok_) {
                    if (c == '{') { string(); expect(':'); }
                    skipValue(depth + 1);
                    if (consume(',')) continue;
                    expect(close);
                    return;
                }
                return;
            }
            if (s_.compare(pos_, 4, "true") == 0) { pos_ += 4; return; }
            if (s_.compare(pos_, 5, "false") == 0) { pos_ += 5; return; }
            if (s_.compare(pos_, 4, "null") == 0) { pos_ += 4; return; }
            skipNumber();
        }

    private:
        bool readHexCodeUnit(uint32_t& value) {
            if (pos_ + 4 > s_.size()) return false;
            value = 0;
            for (int i = 0; i < 4; ++i) {
                char h = s_[pos_++];
                value <<= 4;
                if (h >= '0' && h <= '9') value |= static_cast<uint32_t>(h - '0');
                else if (h >= 'a' && h <= 'f') value |= static_cast<uint32_t>(h - 'a' + 10);
                else if (h >= 'A' && h <= 'F') value |= static_cast<uint32_t>(h - 'A' + 10);
                else return false;
            }
            return true;
        }

        static void appendUtf8(std::string& out, uint32_t codePoint) {
            if (codePoint <= 0x7F) {
                out += static_cast<char>(codePoint);
            } else if (codePoint <= 0x7FF) {
                out += static_cast<char>(0xC0 | (codePoint >> 6));
                out += static_cast<char>(0x80 | (codePoint & 0x3F));
            } else if (codePoint <= 0xFFFF) {
                out += static_cast<char>(0xE0 | (codePoint >> 12));
                out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (codePoint & 0x3F));
            } else {
                out += static_cast<char>(0xF0 | (codePoint >> 18));
                out += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (codePoint & 0x3F));
            }
        }

        void skipNumber() {
            if (pos_ < s_.size() && s_[pos_] == '-') ++pos_;
            if (pos_ >= s_.size()) { fail(); return; }
            if (s_[pos_] == '0') {
                ++pos_;
            } else if (s_[pos_] >= '1' && s_[pos_] <= '9') {
                do { ++pos_; } while (pos_ < s_.size() && s_[pos_] >= '0' && s_[pos_] <= '9');
            } else {
                fail();
                return;
            }
            if (pos_ < s_.size() && s_[pos_] == '.') {
                ++pos_;
                size_t fractionStart = pos_;
                while (pos_ < s_.size() && s_[pos_] >= '0' && s_[pos_] <= '9') ++pos_;
                if (pos_ == fractionStart) { fail(); return; }
            }
            if (pos_ < s_.size() && (s_[pos_] == 'e' || s_[pos_] == 'E')) {
                ++pos_;
                if (pos_ < s_.size() && (s_[pos_] == '+' || s_[pos_] == '-')) ++pos_;
                size_t exponentStart = pos_;
                while (pos_ < s_.size() && s_[pos_] >= '0' && s_[pos_] <= '9') ++pos_;
                if (pos_ == exponentStart) fail();
            }
        }

        const std::string& s_;
        size_t pos_ = 0;
        bool   ok_  = true;
    };

    /**
     * @brief Iterate a `{ "name": { "key": value, ... }, ... }` document.
     *
     * @param onEntry  Called as onEntry(name, reader, key) for each inner key;
     *                 must consume exactly one value.
     * @param onDone   Called as onDone(name) after each inner object closes.
     * @return False on any syntax error.
     */
    template <typename OnKey, typename OnDone>
    static bool parseDocument(const std::string& json, OnKey onKey, OnDone onDone) {
        Reader r(json);
        r.expect('{');
        if (r.consume('}')) return r.atEnd();
        while (r.ok()) {
            std::string name = r.string();
            r.expect(':');
            r.expect('{');
            if (!r.consume('}')) {
                while (r.ok()) {
                    std::string key = r.string();
                    r.expect(':');
                    if (!r.ok()) break;
                    onKey(name, r, key);
                    if (r.consume(',')) continue;
                    r.expect('}');
                    break;
                }
            }
            if (!r.ok()) break;
            onDone(name);
            if (r.consume(',')) continue;
            r.expect('}');
            break;
        }
        return r.ok() && r.atEnd();
    }

    // ── File loaders ──────────────────────────────────────────────────────────

    bool readFile(const std::filesystem::path& path, std::string& out) {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) return false;
        auto size = std::filesystem::file_size(path, ec);
        if (ec) { lastError_ = "cannot stat " + path.u8string(); return false; }
        if (size > kProfileFileMaxBytes) {
            lastError_ = path.u8string() + " exceeds size limit";
            return false;
        }
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) { lastError_ = "cannot open " + path.u8string(); return false; }
        out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        // Tolerate a UTF-8 BOM written by some editors.
        if (out.size() >= 3 && out.compare(0, 3, "\xEF\xBB\xBF") == 0) out.erase(0, 3);
        return true;
    }

    size_t loadUserProfiles(const std::filesystem::path& path) {
        std::string json;
        if (!readFile(path, json)) return 0;
        return parseUserProfiles(json, path.u8string());
    }

    size_t loadTemplates(const std::filesystem::path& path) {
        std::string json;
        if (!readFile(path, json)) return 0;
        return parseTemplates(json, path.u8string());
    }

public:
    /**
     * @brief Parse a device_profiles.json document held in memory and merge it.
     * @param json    Document text.
     * @param origin  Name used in error messages (e.g. the file path).
     * @return Number of profiles merged; 0 with lastError() set if malformed.
     */
    size_t parseUserProfiles(const std::string& json,
                             const std::string& origin = "input") {
        std::unordered_map<std::string, DeviceProfile> parsed;
        DeviceProfile current;
        bool ok = parseDocument(json,
            [&](const std::string&, Reader& r, const std::string& key) {
                if (key == "templateName")       current.templateName = r.string();
                else if (key == "subscriptions") current.subscriptions = r.u16Array();
                else if (key == "wantsAll")      current.wantsAll = r.boolean().value_or(false);
                else                             r.skipValue();
            },
            [&](const std::string& name) {
                if (IsValidDeviceName(name)) parsed[name] = std::move(current);
                current = DeviceProfile{};
            });
        if (!ok) {
            lastError_ = "malformed JSON in " + origin;
            return 0;
        }
        size_t count = parsed.size();
        for (auto& kv : parsed) userProfiles_[kv.first] = std::move(kv.second);
        return count;
    }

    /**
     * @brief Parse a panels.json template document held in memory and merge it.
     * @param json    Document text.
     * @param origin  Name used in error messages (e.g. the file path).
     * @return Number of templates merged; 0 with lastError() set if malformed.
     */
    size_t parseTemplates(const std::string& json,
                             const std::string& origin = "input") {
        std::unordered_map<std::string, TemplateEntry> parsed;
        TemplateEntry current;
        bool ok = parseDocument(json,
            [&](const std::string&, Reader& r, const std::string& key) {
                if (key == "description")        current.description = r.string();
                else if (key == "subscriptions") current.subscriptions = r.u16Array();
                else                             r.skipValue();
            },
            [&](const std::string& name) {
                if (!name.empty()) parsed[name] = std::move(current);
                current = TemplateEntry{};
            });
        if (!ok) {
            lastError_ = "malformed JSON in " + origin;
            return 0;
        }
        size_t count = parsed.size();
        for (auto& kv : parsed) templates_[kv.first] = std::move(kv.second);
        return count;
    }

private:
    /**
     * @brief Write current userProfiles_ back to device_profiles.json.
     * @return True if the file was fully written and moved into place.
     */
    bool flushUserProfiles() {
        if (exeDir_.empty()) { lastError_ = "profile store not loaded"; return false; }
        const std::filesystem::path path = exeDir_ / "device_profiles.json";
        std::filesystem::path tmp = path;
        tmp += ".tmp";

        // Sort for stable, diff-friendly output.
        std::vector<const std::pair<const std::string, DeviceProfile>*> entries;
        entries.reserve(userProfiles_.size());
        for (const auto& kv : userProfiles_) entries.push_back(&kv);
        std::sort(entries.begin(), entries.end(),
                  [](const auto* a, const auto* b) { return a->first < b->first; });

        {
            std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
            if (!f.is_open()) { lastError_ = "cannot write " + tmp.u8string(); return false; }

            f << "{\n";
            bool firstDevice = true;
            for (const auto* kv : entries) {
                const auto& profile = kv->second;
                if (!firstDevice) f << ",\n";
                firstDevice = false;
                f << "  \"" << JsonEscape(kv->first) << "\": {\n";
                f << "    \"templateName\": \"" << JsonEscape(profile.templateName) << "\",\n";
                f << "    \"subscriptions\": [";
                for (size_t i = 0; i < profile.subscriptions.size(); ++i) {
                    if (i > 0) f << ", ";
                    f << profile.subscriptions[i];
                }
                f << "],\n";
                f << "    \"wantsAll\": " << (profile.wantsAll ? "true" : "false") << "\n";
                f << "  }";
            }
            f << "\n}\n";
            f.flush();
            if (!f.good()) {
                lastError_ = "write failed for " + tmp.u8string();
                f.close();
                std::error_code ignored;
                std::filesystem::remove(tmp, ignored);
                return false;
            }
        }

        // Replace the original only after the temp file is complete.
        std::error_code ec;
        std::filesystem::rename(tmp, path, ec);
        if (ec) {
            lastError_ = "cannot replace " + path.u8string() + ": " + ec.message();
            std::error_code ignored;
            std::filesystem::remove(tmp, ignored);
            return false;
        }
        return true;
    }

    // ── Members ───────────────────────────────────────────────────────────────

    std::filesystem::path                           exeDir_;
    std::unordered_map<std::string, DeviceProfile>  userProfiles_; ///< Loaded from device_profiles.json
    std::unordered_map<std::string, TemplateEntry>  templates_;    ///< Loaded from templates/panels.json
    std::string                                     lastError_;
};

} // namespace dcsbios
