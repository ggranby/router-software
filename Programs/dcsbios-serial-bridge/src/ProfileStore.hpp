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
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>

namespace dcsbios {

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
    size_t load(const std::wstring& exeDir) {
        exeDir_ = exeDir;
        size_t total = 0;
        total += loadUserProfiles(exeDir + L"\\device_profiles.json");
        total += loadTemplates(exeDir + L"\\templates\\panels.json");
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
     * via the UI.  The file is written atomically (write to temp, rename).
     *
     * @param deviceName  Device name key (from handshake).
     * @param profile     Profile to store.
     */
    void save(const std::string& deviceName, const DeviceProfile& profile) {
        userProfiles_[deviceName] = profile;
        flushUserProfiles();
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

private:
    // ── Internal types ────────────────────────────────────────────────────────

    struct TemplateEntry {
        std::string           description;
        std::vector<uint16_t> subscriptions;
    };

    // ── Minimal JSON parser helpers ───────────────────────────────────────────
    // Hornet Link has no JSON library dependency, so we use a hand-written
    // parser for the small, well-structured files we control.

    /**
     * @brief Skip whitespace in string view.
     */
    static size_t skipWs(const std::string& s, size_t p) {
        while (p < s.size() && (s[p] == ' ' || s[p] == '\t' || s[p] == '\r' || s[p] == '\n'))
            ++p;
        return p;
    }

    /**
     * @brief Parse a JSON string value starting at the opening `"`.
     *
     * @return Parsed string (without quotes); @p pos advanced past closing `"`.
     */
    static std::string parseString(const std::string& s, size_t& pos) {
        if (pos >= s.size() || s[pos] != '"') return {};
        ++pos; // skip opening quote
        std::string out;
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\' && pos + 1 < s.size()) { ++pos; out += s[pos]; }
            else out += s[pos];
            ++pos;
        }
        if (pos < s.size()) ++pos; // skip closing quote
        return out;
    }

    /**
     * @brief Parse a JSON integer value.
     *
     * @return Parsed integer; @p pos advanced past the number.
     */
    static int parseInteger(const std::string& s, size_t& pos) {
        bool neg = (pos < s.size() && s[pos] == '-');
        if (neg) ++pos;
        int n = 0;
        while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') {
            n = n * 10 + (s[pos] - '0');
            ++pos;
        }
        return neg ? -n : n;
    }

    /**
     * @brief Parse a JSON array of integers into a uint16_t vector.
     */
    static std::vector<uint16_t> parseU16Array(const std::string& s, size_t& pos) {
        std::vector<uint16_t> out;
        pos = skipWs(s, pos);
        if (pos >= s.size() || s[pos] != '[') return out;
        ++pos;
        while (true) {
            pos = skipWs(s, pos);
            if (pos >= s.size() || s[pos] == ']') { if (pos < s.size()) ++pos; break; }
            if (s[pos] >= '0' && s[pos] <= '9') {
                int v = parseInteger(s, pos);
                if (v >= 0 && v <= 0xFFFF) out.push_back(static_cast<uint16_t>(v));
            }
            pos = skipWs(s, pos);
            if (pos < s.size() && s[pos] == ',') ++pos;
        }
        return out;
    }

    // ── File loaders ──────────────────────────────────────────────────────────

    size_t loadUserProfiles(const std::wstring& path) {
        std::ifstream f(path);
        if (!f.is_open()) return 0;
        std::string json((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());

        size_t count = 0;
        size_t pos = 0;
        pos = skipWs(json, pos);
        if (pos >= json.size() || json[pos] != '{') return 0;
        ++pos;

        while (true) {
            pos = skipWs(json, pos);
            if (pos >= json.size() || json[pos] == '}') break;
            if (json[pos] != '"') { ++pos; continue; }

            std::string deviceName = parseString(json, pos);
            pos = skipWs(json, pos);
            if (pos >= json.size() || json[pos] != ':') break;
            ++pos;
            pos = skipWs(json, pos);
            if (pos >= json.size() || json[pos] != '{') break;
            ++pos;

            DeviceProfile p;
            while (true) {
                pos = skipWs(json, pos);
                if (pos >= json.size() || json[pos] == '}') { if (pos < json.size()) ++pos; break; }
                if (json[pos] != '"') { ++pos; continue; }
                std::string key = parseString(json, pos);
                pos = skipWs(json, pos);
                if (pos < json.size() && json[pos] == ':') ++pos;
                pos = skipWs(json, pos);

                if (key == "templateName") {
                    p.templateName = parseString(json, pos);
                } else if (key == "subscriptions") {
                    p.subscriptions = parseU16Array(json, pos);
                } else if (key == "wantsAll") {
                    // parse bool
                    if (json.compare(pos, 4, "true") == 0)  { p.wantsAll = true;  pos += 4; }
                    if (json.compare(pos, 5, "false") == 0) { p.wantsAll = false; pos += 5; }
                } else {
                    // Skip unknown value
                    while (pos < json.size() && json[pos] != ',' && json[pos] != '}') ++pos;
                }
                pos = skipWs(json, pos);
                if (pos < json.size() && json[pos] == ',') ++pos;
            }

            if (!deviceName.empty()) {
                userProfiles_[deviceName] = std::move(p);
                ++count;
            }
            pos = skipWs(json, pos);
            if (pos < json.size() && json[pos] == ',') ++pos;
        }
        return count;
    }

    size_t loadTemplates(const std::wstring& path) {
        std::ifstream f(path);
        if (!f.is_open()) return 0;
        std::string json((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());

        size_t count = 0;
        size_t pos = 0;
        pos = skipWs(json, pos);
        if (pos >= json.size() || json[pos] != '{') return 0;
        ++pos;

        while (true) {
            pos = skipWs(json, pos);
            if (pos >= json.size() || json[pos] == '}') break;
            if (json[pos] != '"') { ++pos; continue; }

            std::string templateName = parseString(json, pos);
            pos = skipWs(json, pos);
            if (pos >= json.size() || json[pos] != ':') break;
            ++pos;
            pos = skipWs(json, pos);
            if (pos >= json.size() || json[pos] != '{') break;
            ++pos;

            TemplateEntry t;
            while (true) {
                pos = skipWs(json, pos);
                if (pos >= json.size() || json[pos] == '}') { if (pos < json.size()) ++pos; break; }
                if (json[pos] != '"') { ++pos; continue; }
                std::string key = parseString(json, pos);
                pos = skipWs(json, pos);
                if (pos < json.size() && json[pos] == ':') ++pos;
                pos = skipWs(json, pos);

                if (key == "description") {
                    t.description = parseString(json, pos);
                } else if (key == "subscriptions") {
                    t.subscriptions = parseU16Array(json, pos);
                } else {
                    while (pos < json.size() && json[pos] != ',' && json[pos] != '}') ++pos;
                }
                pos = skipWs(json, pos);
                if (pos < json.size() && json[pos] == ',') ++pos;
            }

            if (!templateName.empty()) {
                templates_[templateName] = std::move(t);
                ++count;
            }
            pos = skipWs(json, pos);
            if (pos < json.size() && json[pos] == ',') ++pos;
        }
        return count;
    }

    /**
     * @brief Write current userProfiles_ back to device_profiles.json.
     */
    void flushUserProfiles() {
        if (exeDir_.empty()) return;
        std::wstring path = exeDir_ + L"\\device_profiles.json";
        std::wstring tmp  = path + L".tmp";

        std::ofstream f(tmp);
        if (!f.is_open()) return;

        f << "{\n";
        bool firstDevice = true;
        for (const auto& [name, profile] : userProfiles_) {
            if (!firstDevice) f << ",\n";
            firstDevice = false;
            f << "  \"" << name << "\": {\n";
            f << "    \"templateName\": \"" << profile.templateName << "\",\n";
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
        f.close();

        // Atomic rename
        MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);
    }

    // ── Members ───────────────────────────────────────────────────────────────

    std::wstring                                    exeDir_;
    std::unordered_map<std::string, DeviceProfile>  userProfiles_; ///< Loaded from device_profiles.json
    std::unordered_map<std::string, TemplateEntry>  templates_;    ///< Loaded from templates/panels.json
};

} // namespace dcsbios
