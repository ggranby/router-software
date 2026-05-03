#pragma once
/**
 * @file SimSource.hpp
 * @brief ISimSource — pluggable simulator data-source interface.
 *
 * @details
 * Hornet Link supports multiple simulator back-ends (DCS via Hornet Link's own
 * Lua export script, DCS via the legacy DCS-BIOS UDP stream, MSFS 2024 via
 * SimConnect, and a file-replay source for offline testing). All back-ends
 * implement this interface so BridgeController remains source-agnostic.
 *
 * ### Lifecycle
 * 1. Construct the concrete source (e.g. `DcsBiosSource`, `DcsDirectSource`).
 * 2. Install the `onFrameSync` callback — called on the source's internal
 *    worker thread after each complete simulator frame.
 * 3. Call `connect(stateMap, logHwnd)`.  The source begins receiving data and
 *    writing it into the shared 64 KB BiosStateMap.
 * 4. Call `disconnect()` when the session ends (or let the destructor do it).
 * 5. Call `sendImport(cmd)` from any thread to forward a hardware import
 *    command to the simulator.
 *
 * ### Threading contract
 * - `connect()` and `disconnect()` are called from the UI thread.
 * - `onFrameSync` is invoked on the source's internal worker thread.
 * - `sendImport()` may be called from any thread; implementations must be
 *   thread-safe.
 * - The caller is responsible for any synchronisation needed between the
 *   worker thread (which writes into BiosStateMap) and the consumer of
 *   dirty addresses (which reads from it inside `onFrameSync`).  In practice
 *   BridgeController's `OnFrameSync` calls `BiosStateMap::takeDirty()` which
 *   is safe because both operations run on the worker thread.
 *
 * @copyright Copyright 2016-2026 Hornet Link contributors.
 *            Licensed under the Apache License, Version 2.0.
 */

#include "BiosProtocol.hpp"

#include <functional>
#include <string>
#include <windows.h>   // HWND

namespace dcsbios {

// ─────────────────────────────────────────────────────────────────────────────
// ISimSource
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for a simulator data source.
 *
 * Concrete implementations:
 * | Class               | Description                                          |
 * |---------------------|------------------------------------------------------|
 * | DcsBiosSource       | Receives the DCS-BIOS UDP/TCP export stream          |
 * | DcsDirectSource     | Receives Hornet Link's own DCS Lua UDP export        |
 * | ReplayFileSource    | Replays a recorded `dcsbios_data.json` at real speed |
 * | MsfsSource          | MSFS 2024 stub (not yet implemented)                 |
 */
class ISimSource {
public:
    virtual ~ISimSource() = default;

    // Non-copyable — each source owns OS handles
    ISimSource()                               = default;
    ISimSource(const ISimSource&)              = delete;
    ISimSource& operator=(const ISimSource&)   = delete;

    // ── Callback installed by the consumer ──────────────────────────────────

    /**
     * @brief Invoked on the source's worker thread after each complete frame.
     *
     * The callback receives the list of even byte addresses that changed since
     * the previous frame (the same list returned by BiosStateMap::takeDirty()).
     * The shared BiosStateMap has already been updated when this fires.
     *
     * Set this before calling connect().
     */
    std::function<void(const std::vector<uint16_t>& dirty)> onFrameSync;

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /**
     * @brief Start the source and begin writing into @p stateMap.
     *
     * The source takes a reference to the shared BiosStateMap; the caller
     * guarantees the map outlives the source.
     *
     * @param stateMap  Shared 64 KB state mirror owned by BridgeController.
     * @param logHwnd   Window that receives PostLog() kLogMessage messages.
     * @return True on success; false if the source cannot initialise.
     */
    virtual bool connect(BiosStateMap& stateMap, HWND logHwnd) = 0;

    /**
     * @brief Stop the source and release all OS handles.
     *
     * Blocks until the internal worker thread has exited. Safe to call if
     * never connected or after a previous call to disconnect().
     */
    virtual void disconnect() = 0;

    /**
     * @brief Return true if the source is actively connected.
     *
     * May be called from any thread. The return value is advisory.
     */
    virtual bool isConnected() const = 0;

    // ── Import forwarding ─────────────────────────────────────────────────

    /**
     * @brief Forward a hardware import command to the simulator.
     *
     * For DCS sources this sends a plain-text UDP datagram to
     * `127.0.0.1:7778`.  For replay and stub sources this is a no-op.
     *
     * Thread-safe — may be called from the BridgeController's read-device
     * threads while the source's worker thread is running.
     *
     * @param cmd  The parsed import command from the hardware device.
     */
    virtual void sendImport(const ImportCommand& cmd) = 0;

    // ── Identification ────────────────────────────────────────────────────

    /**
     * @brief Short display name shown in the UI source selector combo box.
     * @return Pointer to a static wide-character string; lifetime is the
     *         lifetime of the source object.
     */
    virtual const wchar_t* displayName() const = 0;

    /**
     * @brief True when this source is a stub and import forwarding is disabled.
     *
     * Used by BridgeController to warn the operator that hardware→sim commands
     * will be dropped.
     */
    virtual bool isStub() const { return false; }
};

} // namespace dcsbios
