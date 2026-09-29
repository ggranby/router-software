#pragma once
/**
 * @file LogPost.hpp
 * @brief Shared helper for posting log lines from any thread to the UI window.
 *
 * @details
 * Log lines travel to the UI as a heap-allocated `std::wstring` whose address
 * is sent as the LPARAM of a kLogMessage. The receiving WndProc takes
 * ownership and deletes it. If PostMessage fails (window destroyed, queue
 * full) ownership stays with the sender and the string is freed here, so no
 * payload is leaked.
 *
 * @copyright Copyright 2016-2026 Hornet Link contributors.
 *            Licensed under the Apache License, Version 2.0.
 */

#include <memory>
#include <string>
#include <windows.h>

namespace dcsbios {

/// Window message carrying a heap-allocated std::wstring* log line in LPARAM.
constexpr UINT kLogMessage = WM_APP + 1;

/**
 * @brief Post @p line (already formatted, including line ending) to @p hwnd.
 *
 * Safe to call from any thread. Does nothing when @p hwnd is null.
 *
 * @return True if the message was queued (receiver now owns the payload).
 */
inline bool PostLogLine(HWND hwnd, std::wstring line) {
    if (!hwnd) return false;
    auto payload = std::make_unique<std::wstring>(std::move(line));
    if (!PostMessageW(hwnd, kLogMessage, 0, reinterpret_cast<LPARAM>(payload.get())))
        return false; // unique_ptr frees the payload
    payload.release(); // ownership transferred to the WndProc
    return true;
}

/**
 * @brief Free any kLogMessage payloads still queued for @p hwnd.
 *
 * Call while handling WM_DESTROY so lines posted during shutdown are not
 * leaked.
 */
inline void DrainLogMessages(HWND hwnd) {
    MSG msg;
    while (PeekMessageW(&msg, hwnd, kLogMessage, kLogMessage, PM_REMOVE)) {
        delete reinterpret_cast<std::wstring*>(msg.lParam);
    }
}

} // namespace dcsbios
