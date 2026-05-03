#pragma once
/**
 * @file DcsBiosSource.hpp
 * @brief DCS-BIOS UDP/TCP export stream source (backward-compatible).
 *
 * @details
 * Implements ISimSource by receiving the DCS-BIOS multicast UDP stream
 * (239.255.50.10:5010) or the TCP stream (127.0.0.1:7778), decoding it
 * through ExportParser, and updating the shared BiosStateMap.
 *
 * This is the "secondary / backward-compat" path described in plan.md Phase 1.
 * It preserves the exact behaviour of the original BridgeController::RunLoop()
 * so existing DCS-BIOS installations continue to work without change.
 *
 * ### Thread model
 * | Thread      | Responsibility                              |
 * |-------------|---------------------------------------------|
 * | UI thread   | connect() / disconnect()                    |
 * | Worker      | UDP/TCP recv → ExportParser → onFrameSync   |
 *
 * The worker thread is started by connect() and joins in disconnect().
 *
 * @copyright Copyright 2016-2026 Hornet Link contributors.
 *            Licensed under the Apache License, Version 2.0.
 */

#include "SimSource.hpp"
#include "BiosProtocol.hpp"

#include <atomic>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

namespace dcsbios {

// ─────────────────────────────────────────────────────────────────────────────
// DcsBiosSource
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief ISimSource implementation that receives the DCS-BIOS export stream.
 *
 * @details
 * ### UDP mode (default)
 * Binds to `INADDR_ANY:5010` and joins the `239.255.50.10` multicast group.
 * After each datagram `ExportParser::flushFrame()` is called so the parser
 * treats the datagram boundary as a frame boundary, matching DCS-BIOS output
 * semantics.
 *
 * ### TCP mode
 * Connects to `127.0.0.1:7778`.  The DCS-BIOS TCP server emits sync bytes
 * (`0x55 0x55 0x55 0x55`) naturally in the stream, so no additional flush is
 * needed.
 *
 * ### Import forwarding
 * sendImport() sends plain-text UDP datagrams to `127.0.0.1:7778` in the
 * standard DCS-BIOS format (`IDENTIFIER SET_STATE value\n`).
 */
class DcsBiosSource : public ISimSource {
public:
    // ── Configuration ────────────────────────────────────────────────────────

    /// Network mode selector
    enum class Mode { Udp, Tcp };

    /**
     * @brief Construct a source in the specified network mode.
     * @param mode  Udp → join multicast group; Tcp → connect to loopback server.
     */
    explicit DcsBiosSource(Mode mode = Mode::Udp) : mode_(mode) {}

    ~DcsBiosSource() override { disconnect(); }

    // ── ISimSource ────────────────────────────────────────────────────────────

    const wchar_t* displayName() const override {
        return mode_ == Mode::Udp
            ? L"DCS — DCS-BIOS Stream (UDP)"
            : L"DCS — DCS-BIOS Stream (TCP)";
    }

    bool isConnected() const override { return running_; }

    /**
     * @brief Initialise WinSock, create the receive socket, and start the worker.
     *
     * @param stateMap  Shared 64 KB state map owned by BridgeController.
     * @param logHwnd   Log target window (receives PostLog kLogMessage messages).
     * @return True on success.
     */
    bool connect(BiosStateMap& stateMap, HWND logHwnd) override {
        if (running_) return false;
        stateMap_ = &stateMap;
        logHwnd_  = logHwnd;

        WSADATA wd = {};
        if (WSAStartup(MAKEWORD(2, 2), &wd) != 0) {
            postLog(L"DcsBiosSource: WSAStartup failed.");
            return false;
        }
        winsockInitialised_ = true;

        InitImportSocket();

        running_ = true;
        backoffMs_ = 1000;
        worker_ = std::thread(&DcsBiosSource::runLoop, this);
        return true;
    }

    /**
     * @brief Signal the worker to stop, join it, and release all OS handles.
     */
    void disconnect() override {
        bool wasRunning = running_.exchange(false);
        if (recvSocket_ != INVALID_SOCKET) {
            closesocket(recvSocket_);
            recvSocket_ = INVALID_SOCKET;
        }
        if (worker_.joinable()) worker_.join();
        if (importSocket_ != INVALID_SOCKET) {
            closesocket(importSocket_);
            importSocket_ = INVALID_SOCKET;
        }
        if (winsockInitialised_) { WSACleanup(); winsockInitialised_ = false; }
        if (wasRunning) postLog(L"DCS-BIOS source disconnected.");
    }

    /**
     * @brief Forward a hardware import command to DCS via UDP.
     *
     * Thread-safe — sendto() is used without locking because the import
     * socket is write-only and the destination address is fixed.
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
        // kLogMessage = WM_APP + 1 — matches main.cpp constant
        constexpr int kLogMessage = WM_APP + 1;
        auto* payload = new std::wstring(msg + L"\r\n");
        PostMessage(logHwnd_, kLogMessage, 0, reinterpret_cast<LPARAM>(payload));
    }

    void InitImportSocket() {
        importSocket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (importSocket_ == INVALID_SOCKET) {
            postLog(L"DcsBiosSource: import socket failed (device→DCS disabled).");
            return;
        }
        importAddr_ = {};
        importAddr_.sin_family = AF_INET;
        importAddr_.sin_port   = htons(7778);
        inet_pton(AF_INET, "127.0.0.1", &importAddr_.sin_addr);
    }

    bool initUdpSocket() {
        recvSocket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (recvSocket_ == INVALID_SOCKET) {
            postLog(L"DcsBiosSource: failed to create UDP socket.");
            return false;
        }
        BOOL reuse = TRUE;
        setsockopt(recvSocket_, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&reuse), sizeof(reuse));
        DWORD rcvTimeout = kSocketReceiveTimeoutMs;
        setsockopt(recvSocket_, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&rcvTimeout), sizeof(rcvTimeout));

        sockaddr_in addr = {};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(5010);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(recvSocket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            postLog(L"DcsBiosSource: failed to bind UDP socket to :5010.");
            return false;
        }
        ip_mreq membership = {};
        inet_pton(AF_INET, "239.255.50.10", &membership.imr_multiaddr);
        membership.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(recvSocket_, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                       reinterpret_cast<const char*>(&membership), sizeof(membership)) == SOCKET_ERROR) {
            postLog(L"DcsBiosSource: failed to join multicast group.");
            return false;
        }
        postLog(L"DCS-BIOS UDP source active on 239.255.50.10:5010.");
        return true;
    }

    bool initTcpSocket() {
        recvSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (recvSocket_ == INVALID_SOCKET) {
            postLog(L"DcsBiosSource: failed to create TCP socket.");
            return false;
        }
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(7778);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        const int connectResult = ::connect(recvSocket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        if (connectResult != 0) {
            postLog(L"DcsBiosSource: TCP connect to 127.0.0.1:7778 failed.");
            return false;
        }
        postLog(L"DCS-BIOS TCP source connected to 127.0.0.1:7778.");
        return true;
    }

    // ── Worker thread ─────────────────────────────────────────────────────────

    /**
     * @brief Main receive loop — mirrors the original BridgeController::RunLoop().
     *
     * Runs until running_ is false. On socket error or disconnection it closes
     * the socket and retries with exponential backoff (1 s → 5 s cap).
     */
    void runLoop() {
        // Create a fresh ExportParser wrapping the shared stateMap each connect.
        ExportParser parser(*stateMap_);
        parser.onFrameSync = [this]() {
            auto dirty = stateMap_->takeDirty();
            if (onFrameSync) onFrameSync(dirty);
        };

        std::vector<char> buf(kExportDatagramMaxBytes);

        while (running_) {
            if (recvSocket_ == INVALID_SOCKET) {
                bool ok = (mode_ == Mode::Udp) ? initUdpSocket() : initTcpSocket();
                if (!ok) {
                    std::wstring m = L"DcsBiosSource: retrying in ";
                    m += std::to_wstring(backoffMs_ / 1000) + L"s.";
                    postLog(m);
                    Sleep(backoffMs_);
                    backoffMs_ = std::min(backoffMs_ * 2, 5000);
                    continue;
                }
                backoffMs_ = 1000;
            }

            int received = recv(recvSocket_, buf.data(), static_cast<int>(buf.size()), 0);
            if (!running_) break;

            if (received == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAETIMEDOUT) continue;
                std::wstring m = L"DcsBiosSource: socket error ";
                m += std::to_wstring(err) + L". Reconnecting.";
                postLog(m);
                closesocket(recvSocket_);
                recvSocket_ = INVALID_SOCKET;
                continue;
            }
            if (received == 0) {
                postLog(L"DcsBiosSource: connection closed. Reconnecting.");
                closesocket(recvSocket_);
                recvSocket_ = INVALID_SOCKET;
                continue;
            }

            parser.processBytes(reinterpret_cast<const uint8_t*>(buf.data()),
                                 static_cast<size_t>(received));
            if (mode_ == Mode::Udp) parser.flushFrame();
        }

        if (recvSocket_ != INVALID_SOCKET) {
            closesocket(recvSocket_);
            recvSocket_ = INVALID_SOCKET;
        }
    }

    // ── Constants ─────────────────────────────────────────────────────────────

    /// Maximum bytes consumed from the network in a single recv() call.
    static constexpr int   kExportDatagramMaxBytes = 4096;
    /// SO_RCVTIMEO so the loop can check running_ while DCS is idle.
    static constexpr DWORD kSocketReceiveTimeoutMs = 5000;

    // ── Members ───────────────────────────────────────────────────────────────

    Mode                        mode_{Mode::Udp};
    BiosStateMap*               stateMap_            = nullptr;
    HWND                        logHwnd_             = nullptr;
    std::atomic<bool>           running_{false};
    std::thread                 worker_;
    SOCKET                      recvSocket_          = INVALID_SOCKET;
    SOCKET                      importSocket_        = INVALID_SOCKET;
    sockaddr_in                 importAddr_          = {};
    std::atomic<bool>           winsockInitialised_{false};
    int                         backoffMs_           = 1000; ///< Reconnect back-off (worker only)
};

} // namespace dcsbios
