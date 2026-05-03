#pragma once
/**
 * @file DcsDirectSource.hpp
 * @brief Hornet Link's own DCS Lua export source (primary path).
 *
 * @details
 * This is the **primary** data source for Hornet Link.  Instead of depending
 * on the DCS-BIOS Lua framework being installed, Hornet Link ships its own
 * lightweight Lua export script (`lua/HornetLinkExport.lua`) that:
 *
 *  1. Reads the per-module address map from `lua/modules/<ModuleName>.lua`.
 *  2. Each simulation tick (`Export.lua` is called by DCS every ~30 ms).
 *  3. Builds a binary export packet in the same wire format as DCS-BIOS:
 *     sync frame + write records.
 *  4. Sends the UDP datagram to `127.0.0.1:42002` (Hornet Link port).
 *
 * DcsDirectSource listens on that port, feeds the bytes into the standard
 * ExportParser, and fires `onFrameSync` — exactly as DcsBiosSource does.
 *
 * ### Architecture
 * - **Module-agnostic base:** `HornetLinkExport.lua` handles the export loop
 *   and framing for any aircraft.
 * - **Per-module definitions:** `lua/modules/FA-18C.lua` declares address
 *   ranges for the F/A-18C Hornet.  Adding a new aircraft requires only a
 *   new module definition file; no C++ changes are needed.
 * - **Backward compatibility:** DcsBiosSource is preserved for operators who
 *   prefer to keep DCS-BIOS installed.
 *
 * ### Installation
 * The operator copies `HornetLinkExport.lua` into their DCS `Scripts/`
 * folder (or the `Scripts/Export.lua` calls it via `dofile`). The UI shows
 * the required Lua snippet so the operator can add it manually.
 *
 * @note
 * This source uses the same binary wire format as DCS-BIOS so the existing
 * ExportParser and BiosStateMap are reused without modification.
 *
 * @copyright Copyright 2016-2026 Hornet Link contributors.
 *            Licensed under the Apache License, Version 2.0.
 */

#include "SimSource.hpp"
#include "BiosProtocol.hpp"

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

namespace dcsbios {

// ─────────────────────────────────────────────────────────────────────────────
// DcsDirectSource
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief ISimSource implementation that listens for Hornet Link's own DCS
 *        Lua export UDP datagrams on `127.0.0.1:42002`.
 *
 * @details
 * Wire format is identical to DCS-BIOS (sync frame + write records) so the
 * ExportParser is reused.  Import commands are forwarded to DCS via
 * `127.0.0.1:7778` exactly as DcsBiosSource does.
 *
 * The module-agnostic Lua exporter sends only addresses declared in the active
 * module definition, which is typically a subset of the full 64 KB space.
 * BiosStateMap handles partial writes correctly — unwritten addresses retain
 * their last known value.
 */
class DcsDirectSource : public ISimSource {
public:
    /// UDP port that the Hornet Link Lua exporter sends to (must match lua/HornetLinkExport.lua)
    static constexpr uint16_t kListenPort = 42002;

    explicit DcsDirectSource() = default;
    ~DcsDirectSource() override { disconnect(); }

    // ── ISimSource ────────────────────────────────────────────────────────────

    const wchar_t* displayName() const override {
        return L"DCS — Hornet Link (Lua export)";
    }

    bool isConnected() const override { return running_; }

    /**
     * @brief Open the listen socket and start the worker thread.
     *
     * Binds a UDP socket to `127.0.0.1:42002`.  DCS (via the Lua exporter)
     * must be running and the export script must be active for data to arrive.
     *
     * @param stateMap  Shared 64 KB state map owned by BridgeController.
     * @param logHwnd   Log target window.
     * @return True on success; false if WSAStartup or socket bind fails.
     */
    bool connect(BiosStateMap& stateMap, HWND logHwnd) override {
        if (running_) return false;
        stateMap_ = &stateMap;
        logHwnd_  = logHwnd;

        WSADATA wd = {};
        if (WSAStartup(MAKEWORD(2, 2), &wd) != 0) {
            postLog(L"DcsDirectSource: WSAStartup failed.");
            return false;
        }
        winsockInitialised_ = true;

        if (!initListenSocket()) {
            WSACleanup();
            winsockInitialised_ = false;
            return false;
        }

        initImportSocket();

        postLog(L"Hornet Link source listening on 127.0.0.1:" +
                std::to_wstring(kListenPort) + L".");
        postLog(L"  Add to DCS Scripts/Export.lua:");
        postLog(L"    local hlFile = lfs.writedir()..'Scripts\\\\HornetLinkExport.lua'");
        postLog(L"    if lfs.attributes(hlFile) then dofile(hlFile) end");

        running_ = true;
        worker_ = std::thread(&DcsDirectSource::runLoop, this);
        return true;
    }

    void disconnect() override {
        bool wasRunning = running_.exchange(false);
        if (listenSocket_ != INVALID_SOCKET) {
            closesocket(listenSocket_);
            listenSocket_ = INVALID_SOCKET;
        }
        if (worker_.joinable()) worker_.join();
        if (importSocket_ != INVALID_SOCKET) {
            closesocket(importSocket_);
            importSocket_ = INVALID_SOCKET;
        }
        if (winsockInitialised_) { WSACleanup(); winsockInitialised_ = false; }
        if (wasRunning) postLog(L"Hornet Link source disconnected.");
    }

    /**
     * @brief Forward import command to DCS (same endpoint as DcsBiosSource).
     */
    void sendImport(const ImportCommand& cmd) override {
        if (importSocket_ == INVALID_SOCKET) return;
        std::string line = cmd.toLine();
        sendto(importSocket_, line.c_str(), static_cast<int>(line.size()), 0,
               reinterpret_cast<sockaddr*>(&importAddr_), sizeof(importAddr_));
    }

private:
    // ── Helpers ──────────────────────────────────────────────────────────────

    void postLog(const std::wstring& msg) const {
        if (!logHwnd_) return;
        constexpr int kLogMessage = WM_APP + 1;
        auto* payload = new std::wstring(msg + L"\r\n");
        PostMessage(logHwnd_, kLogMessage, 0, reinterpret_cast<LPARAM>(payload));
    }

    bool initListenSocket() {
        listenSocket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (listenSocket_ == INVALID_SOCKET) {
            postLog(L"DcsDirectSource: failed to create UDP socket.");
            return false;
        }
        BOOL reuse = TRUE;
        setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&reuse), sizeof(reuse));
        DWORD rcvTimeout = 5000; // ms
        setsockopt(listenSocket_, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&rcvTimeout), sizeof(rcvTimeout));

        sockaddr_in addr = {};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(kListenPort);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (bind(listenSocket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            postLog(L"DcsDirectSource: failed to bind UDP socket to 127.0.0.1:" +
                    std::to_wstring(kListenPort) +
                    L" (is another instance running?).");
            closesocket(listenSocket_);
            listenSocket_ = INVALID_SOCKET;
            return false;
        }
        return true;
    }

    void initImportSocket() {
        importSocket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (importSocket_ == INVALID_SOCKET) return;
        importAddr_ = {};
        importAddr_.sin_family = AF_INET;
        importAddr_.sin_port   = htons(7778);
        inet_pton(AF_INET, "127.0.0.1", &importAddr_.sin_addr);
    }

    // ── Worker thread ─────────────────────────────────────────────────────────

    void runLoop() {
        ExportParser parser(*stateMap_);
        parser.onFrameSync = [this]() {
            auto dirty = stateMap_->takeDirty();
            if (onFrameSync) onFrameSync(dirty);
        };

        std::vector<char> buf(4096);

        while (running_) {
            if (listenSocket_ == INVALID_SOCKET) {
                postLog(L"DcsDirectSource: listen socket lost, stopping.");
                break;
            }

            int received = recv(listenSocket_, buf.data(), static_cast<int>(buf.size()), 0);
            if (!running_) break;

            if (received == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAETIMEDOUT) continue;
                postLog(L"DcsDirectSource: socket error " + std::to_wstring(err) + L".");
                break;
            }
            if (received == 0) break;

            parser.processBytes(reinterpret_cast<const uint8_t*>(buf.data()),
                                 static_cast<size_t>(received));
            parser.flushFrame(); // Each UDP datagram = one complete frame
        }

        running_ = false;
    }

    // ── Members ───────────────────────────────────────────────────────────────

    BiosStateMap*       stateMap_          = nullptr;
    HWND                logHwnd_           = nullptr;
    std::atomic<bool>   running_{false};
    std::thread         worker_;
    SOCKET              listenSocket_      = INVALID_SOCKET;
    SOCKET              importSocket_      = INVALID_SOCKET;
    sockaddr_in         importAddr_        = {};
    std::atomic<bool>   winsockInitialised_{false};
};

} // namespace dcsbios
