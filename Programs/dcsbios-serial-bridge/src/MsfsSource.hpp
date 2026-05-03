#pragma once
/**
 * @file MsfsSource.hpp
 * @brief MSFS 2024 simulator source — stub implementation.
 *
 * @details
 * This stub satisfies the ISimSource interface and allows the UI source
 * selector to present "MSFS 2024 (Coming Soon)" as an option.  The
 * implementation does not connect to any simulator; connect() logs an
 * informational message and returns true so the bridge continues with devices
 * in a passthrough state.
 *
 * ### Design notes for a future full implementation
 * Three integration strategies are under consideration:
 *
 * | Strategy     | Pro                                        | Con                              |
 * |--------------|--------------------------------------------|----------------------------------|
 * | SimConnect   | Official Microsoft API; well documented    | Complex async API; Win32 only    |
 * | FSUIPC       | Widely used in sim community               | Third-party dependency           |
 * | MobiFlight   | Open source; hardware-focused              | Requires MobiFlight installation |
 *
 * A full MsfsSource would:
 *  1. Open a SimConnect handle (`SimConnect_Open`).
 *  2. Map cockpit variable names to Hornet Link addresses (or a separate
 *     address space — TBD, since MSFS variables are string-keyed not
 *     address-mapped like DCS-BIOS).
 *  3. Call `SimConnect_RequestDataOnSimObject` to register for periodic
 *     updates, feeding results into a BiosStateMap equivalent.
 *  4. Forward import commands via `SimConnect_TransmitClientEvent`.
 *
 * The 64 KB BiosStateMap address space is DCS-BIOS–specific; MSFS will need
 * its own mapping strategy (deferred — plan.md Phase 1, item 8).
 *
 * @copyright Copyright 2016-2026 Hornet Link contributors.
 *            Licensed under the Apache License, Version 2.0.
 */

#include "SimSource.hpp"
#include "BiosProtocol.hpp"

#include <windows.h>

namespace dcsbios {

// ─────────────────────────────────────────────────────────────────────────────
// MsfsSource (stub)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Placeholder ISimSource for MSFS 2024.
 *
 * All lifecycle methods succeed silently.  sendImport() is a no-op.
 * isStub() returns true so BridgeController can warn the operator.
 */
class MsfsSource : public ISimSource {
public:
    MsfsSource()  = default;
    ~MsfsSource() override = default;

    const wchar_t* displayName() const override {
        return L"MSFS 2024 (Coming Soon)";
    }

    bool isConnected() const override { return connected_; }
    bool isStub()      const override { return true; }

    /**
     * @brief Log a "not yet implemented" notice and pretend to connect.
     *
     * Returns true (not an error) so the bridge can still dispatch to
     * hardware devices in a no-data state while the operator waits.
     */
    bool connect(BiosStateMap& /*stateMap*/, HWND logHwnd) override {
        logHwnd_   = logHwnd;
        connected_ = true;
        postLog(L"MSFS 2024 source: not yet implemented. Hardware devices "
                L"will receive no data. Use DCS sources for live operation.");
        return true;
    }

    void disconnect() override {
        connected_ = false;
    }

    /// No-op — no live simulator to receive imports.
    void sendImport(const ImportCommand&) override {}

private:
    void postLog(const std::wstring& msg) const {
        if (!logHwnd_) return;
        constexpr int kLogMessage = WM_APP + 1;
        auto* payload = new std::wstring(msg + L"\r\n");
        PostMessage(logHwnd_, kLogMessage, 0, reinterpret_cast<LPARAM>(payload));
    }

    HWND logHwnd_   = nullptr;
    bool connected_ = false;
};

} // namespace dcsbios
