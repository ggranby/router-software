#pragma once
/**
 * @file ReplayFileSource.hpp
 * @brief Replay a recorded DCS-BIOS export log for offline testing.
 *
 * @details
 * ReplayFileSource reads a binary capture file produced by `connect-logger.py`
 * and replays it at approximately real-time speed.  This allows developers and
 * testers to exercise the full hub pipeline (parser → state-map → delta frames
 * → serial devices) without a running DCS instance.
 *
 * ### File format
 * The replay file is a raw binary concatenation of DCS-BIOS UDP datagram
 * payloads with lightweight framing:
 * @code
 *   [timestamp_ms: uint32 LE] [payload_len: uint16 LE] [payload: N bytes]
 *   ... repeated for each datagram ...
 * @endcode
 *
 * When `connect-logger.py` is used without the `--binary` flag it writes a
 * plain hex-text format instead.  ReplayFileSource handles the binary format
 * only; the text format is left for external tools.
 *
 * ### Timing
 * Timestamps are wall-clock milliseconds since the start of the capture.
 * ReplayFileSource sleeps between frames to honour the original inter-frame
 * gaps, capped at @ref kMaxInterFrameDelayMs to avoid long pauses after idle
 * periods.  A speed multiplier (`setSpeedMultiplier()`) allows fast-forward
 * replay for CI regression testing.
 *
 * @copyright Copyright 2016-2026 Hornet Link contributors.
 *            Licensed under the Apache License, Version 2.0.
 */

#include "SimSource.hpp"
#include "BiosProtocol.hpp"

#include <atomic>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <windows.h>

namespace dcsbios {

// ─────────────────────────────────────────────────────────────────────────────
// ReplayFileSource
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief ISimSource implementation that replays a recorded binary capture.
 *
 * @details
 * Useful for:
 *  - Automated regression testing (golden-trace comparison)
 *  - Developer debugging without DCS running
 *  - Demonstrating hub behaviour for documentation purposes
 *
 * Import commands from hardware devices are silently dropped (the source is
 * in replay-only mode and there is no live simulator to receive them).
 */
class ReplayFileSource : public ISimSource {
public:
    /// Inter-frame sleep is capped at this value to skip over long idle periods.
    static constexpr int kMaxInterFrameDelayMs = 200;

    /**
     * @brief Construct a replay source.
     * @param filePath  Absolute path to the binary replay file.
     */
    explicit ReplayFileSource(std::wstring filePath)
        : filePath_(std::move(filePath)) {}

    ~ReplayFileSource() override { disconnect(); }

    // ── ISimSource ────────────────────────────────────────────────────────────

    const wchar_t* displayName() const override { return L"Replay File"; }
    bool isConnected()           const override { return running_; }
    bool isStub()                const override { return false; } // replay is fully functional

    /**
     * @brief Open the replay file and start the worker thread.
     *
     * @param stateMap  Shared state map owned by BridgeController.
     * @param logHwnd   Log target window.
     * @return True on success; false if the file cannot be opened.
     */
    bool connect(BiosStateMap& stateMap, HWND logHwnd) override {
        if (running_) return false;
        stateMap_ = &stateMap;
        logHwnd_  = logHwnd;

        file_.open(filePath_, std::ios::binary);
        if (!file_.is_open()) {
            postLog(L"ReplayFileSource: cannot open " + filePath_);
            return false;
        }
        postLog(L"Replay source: " + filePath_);

        running_ = true;
        worker_ = std::thread(&ReplayFileSource::runLoop, this);
        return true;
    }

    void disconnect() override {
        bool wasRunning = running_.exchange(false);
        if (worker_.joinable()) worker_.join();
        if (file_.is_open()) file_.close();
        if (wasRunning) postLog(L"Replay source stopped.");
    }

    /// No-op for replay (there is no live simulator to receive imports).
    void sendImport(const ImportCommand&) override {}

    /**
     * @brief Set playback speed multiplier.
     *
     * @param multiplier  1.0 = real time, 2.0 = 2× speed, 0 = no sleep (as
     *                    fast as possible, useful for CI stress tests).
     */
    void setSpeedMultiplier(double multiplier) { speedMultiplier_ = multiplier; }

private:
    // ── Helpers ──────────────────────────────────────────────────────────────

    void postLog(const std::wstring& msg) const {
        if (!logHwnd_) return;
        constexpr int kLogMessage = WM_APP + 1;
        auto* payload = new std::wstring(msg + L"\r\n");
        PostMessage(logHwnd_, kLogMessage, 0, reinterpret_cast<LPARAM>(payload));
    }

    // Read a little-endian uint32 from file; returns false on EOF/error.
    bool readU32(uint32_t& out) {
        uint8_t b[4];
        if (!file_.read(reinterpret_cast<char*>(b), 4)) return false;
        out = b[0] | (uint32_t(b[1]) << 8) | (uint32_t(b[2]) << 16) | (uint32_t(b[3]) << 24);
        return true;
    }

    // Read a little-endian uint16 from file; returns false on EOF/error.
    bool readU16(uint16_t& out) {
        uint8_t b[2];
        if (!file_.read(reinterpret_cast<char*>(b), 2)) return false;
        out = b[0] | (uint16_t(b[1]) << 8);
        return true;
    }

    // ── Worker thread ─────────────────────────────────────────────────────────

    /**
     * @brief Read frames from the file and feed them into ExportParser at
     *        approximately real-time speed.
     */
    void runLoop() {
        ExportParser parser(*stateMap_);
        parser.onFrameSync = [this]() {
            auto dirty = stateMap_->takeDirty();
            if (onFrameSync) onFrameSync(dirty);
        };

        uint32_t lastTimestampMs = 0;
        bool     firstFrame      = true;

        while (running_ && file_.good()) {
            uint32_t timestampMs = 0;
            uint16_t payloadLen  = 0;

            if (!readU32(timestampMs) || !readU16(payloadLen)) break; // EOF

            // Honour inter-frame delay
            if (!firstFrame) {
                uint32_t delayMs = timestampMs - lastTimestampMs;
                delayMs = std::min(delayMs, static_cast<uint32_t>(kMaxInterFrameDelayMs));
                if (speedMultiplier_ > 0.0 && delayMs > 0) {
                    auto sleepMs = static_cast<DWORD>(
                        static_cast<double>(delayMs) / speedMultiplier_);
                    if (sleepMs > 0) Sleep(sleepMs);
                }
            }
            firstFrame      = false;
            lastTimestampMs = timestampMs;

            if (payloadLen == 0) continue;

            std::vector<uint8_t> payload(payloadLen);
            if (!file_.read(reinterpret_cast<char*>(payload.data()), payloadLen)) break;

            parser.processBytes(payload.data(), payloadLen);
            parser.flushFrame(); // treat each record as a complete frame
        }

        running_ = false;
        postLog(L"Replay source: end of file.");
    }

    // ── Members ───────────────────────────────────────────────────────────────

    std::wstring        filePath_;
    BiosStateMap*       stateMap_     = nullptr;
    HWND                logHwnd_      = nullptr;
    std::atomic<bool>   running_{false};
    std::thread         worker_;
    std::ifstream       file_;
    double              speedMultiplier_ = 1.0; ///< Playback speed (1.0 = real-time)
};

} // namespace dcsbios
