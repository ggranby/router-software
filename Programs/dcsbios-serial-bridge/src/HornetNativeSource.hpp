#pragma once
/**
 * @file HornetNativeSource.hpp
 * @brief UDP transport for the Hornet-native simulator exporter.
 */

#include "HornetNative.hpp"

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include <winsock2.h>
#include <ws2tcpip.h>

namespace hornet_native {

/**
 * @brief Receives HornetLinkNative.lua state datagrams and forwards inputs.
 *
 * The source deliberately owns only the native exporter transport.  Bridge
 * integration can consume the CatalogState and dirty-id callback without
 * coupling this path to the legacy 64 KiB BiosStateMap interface.
 */
class HornetNativeSource {
public:
    static constexpr uint16_t kListenPort = 42003;
    static constexpr uint16_t kInputPort = 42004;

    std::function<void(const DatagramHeader&, const std::vector<uint16_t>&)> onFrameSync;
    std::function<void(const std::string&)> log;

    HornetNativeSource() = default;
    ~HornetNativeSource() { disconnect(); }

    HornetNativeSource(const HornetNativeSource&) = delete;
    HornetNativeSource& operator=(const HornetNativeSource&) = delete;

    bool connect() {
        if (running_) return false;

        WSADATA wd = {};
        if (WSAStartup(MAKEWORD(2, 2), &wd) != 0) {
            emitLog("HornetNativeSource: WSAStartup failed.");
            return false;
        }
        winsockInitialised_ = true;

        listenSocket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        inputSocket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (listenSocket_ == INVALID_SOCKET || inputSocket_ == INVALID_SOCKET) {
            emitLog("HornetNativeSource: failed to create UDP sockets.");
            disconnect();
            return false;
        }

        DWORD timeout = 500;
        setsockopt(listenSocket_, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&timeout), sizeof(timeout));

        sockaddr_in listenAddr = {};
        listenAddr.sin_family = AF_INET;
        listenAddr.sin_port = htons(kListenPort);
        listenAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (bind(listenSocket_, reinterpret_cast<sockaddr*>(&listenAddr),
                 sizeof(listenAddr)) == SOCKET_ERROR) {
            emitLog("HornetNativeSource: failed to bind UDP port 42003.");
            disconnect();
            return false;
        }

        inputAddr_ = {};
        inputAddr_.sin_family = AF_INET;
        inputAddr_.sin_port = htons(kInputPort);
        inet_pton(AF_INET, "127.0.0.1", &inputAddr_.sin_addr);

        state_.clear();
        running_ = true;
        worker_ = std::thread(&HornetNativeSource::runLoop, this);
        emitLog("Hornet native source listening on 127.0.0.1:42003.");
        return true;
    }

    void disconnect() {
        const bool wasRunning = running_.exchange(false);
        if (listenSocket_ != INVALID_SOCKET) {
            closesocket(listenSocket_);
            listenSocket_ = INVALID_SOCKET;
        }
        if (worker_.joinable()) worker_.join();
        if (inputSocket_ != INVALID_SOCKET) {
            closesocket(inputSocket_);
            inputSocket_ = INVALID_SOCKET;
        }
        if (winsockInitialised_) {
            WSACleanup();
            winsockInitialised_ = false;
        }
        if (wasRunning) emitLog("Hornet native source disconnected.");
    }

    bool isConnected() const { return running_; }
    const CatalogState& state() const { return state_; }

    bool sendInput(const hn::InputRecord& input) {
        if (inputSocket_ == INVALID_SOCKET || validateInput(input) != 0) return false;
        const std::string line = formatExporterInput(input);
        return sendto(inputSocket_, line.data(), static_cast<int>(line.size()), 0,
                      reinterpret_cast<const sockaddr*>(&inputAddr_),
                      sizeof(inputAddr_)) == static_cast<int>(line.size());
    }

private:
    void emitLog(const std::string& message) const {
        if (log) log(message);
    }

    void runLoop() {
        std::vector<char> buffer(65535);
        while (running_) {
            const int received = recv(listenSocket_, buffer.data(),
                                      static_cast<int>(buffer.size()), 0);
            if (!running_) break;
            if (received == SOCKET_ERROR) {
                const int error = WSAGetLastError();
                if (error == WSAETIMEDOUT || error == WSAEINTR) continue;
                emitLog("HornetNativeSource: UDP receive failed.");
                break;
            }
            if (received <= 0) continue;

            DatagramHeader header;
            ParseStats stats;
            const ParseResult result = parseExporterDatagram(
                std::string_view(buffer.data(), static_cast<size_t>(received)),
                state_, header, stats);
            if (result != ParseResult::Ok) continue;

            const auto dirty = state_.takeDirty();
            if (onFrameSync) onFrameSync(header, dirty);
        }
        running_ = false;
    }

    CatalogState state_;
    std::atomic<bool> running_{false};
    std::atomic<bool> winsockInitialised_{false};
    SOCKET listenSocket_ = INVALID_SOCKET;
    SOCKET inputSocket_ = INVALID_SOCKET;
    sockaddr_in inputAddr_ = {};
    std::thread worker_;
};

} // namespace hornet_native
