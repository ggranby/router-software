#pragma once
/**
 * @file HornetNativeLink.hpp
 * @brief Transport-neutral v2 HELLO negotiation around LinkSession.
 */

#include "HornetNative.hpp"

#include <chrono>
#include <functional>
#include <string_view>
#include <vector>

namespace hornet_native {

/**
 * @brief Starts protocol v2 discovery and reports when v1 fallback is due.
 *
 * Serial ownership remains with the bridge. The bridge supplies bytes through
 * feed(), forwards encoded bytes through write, and calls poll() from its
 * serial/session loop until either v2 is confirmed or fallback is requested.
 */
class NativeLinkNegotiator {
public:
    static constexpr std::chrono::milliseconds kHelloWindow{200};

    LinkSession session;
    std::function<void()> fallbackToV1;

    void start() {
        fallbackIssued_ = false;
        started_ = true;
        deadline_ = Clock::now() + kHelloWindow;
        session.start();
    }

    void feed(const uint8_t* data, size_t size) {
        session.feed(data, size);
    }

    bool v2Confirmed() const {
        return session.v2Confirmed();
    }

    bool poll() {
        if (!started_ || fallbackIssued_ || session.v2Confirmed()) return false;
        if (Clock::now() < deadline_) return false;
        fallbackIssued_ = true;
        if (fallbackToV1) fallbackToV1();
        return true;
    }

    void reset() {
        started_ = false;
        fallbackIssued_ = false;
    }

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point deadline_{};
    bool started_ = false;
    bool fallbackIssued_ = false;
};

/**
 * @brief Composes exporter parsing, v2 serial framing, and state dispatch.
 *
 * This is the bridge-side runtime seam used by the Windows controller: UDP
 * datagrams update the catalogue, while serial bytes drive LinkSession.
 */
class NativeBridgeRuntime {
public:
    NativeLinkNegotiator link;
    CatalogState state;
    std::function<void(const DatagramHeader&, const std::vector<uint16_t>&)> onState;

    ParseResult ingestDatagram(std::string_view datagram) {
        DatagramHeader header;
        ParseStats stats;
        const ParseResult result = parseExporterDatagram(datagram, state, header, stats);
        if (result != ParseResult::Ok) return result;

        std::vector<uint16_t> dirty = state.takeDirty();
        if (link.session.takeFullStateDue()) dirty = state.allIds();
        link.session.sendState(state, dirty);
        if (onState) onState(header, dirty);
        return result;
    }

    void start() { link.start(); }
    void feedSerial(const uint8_t* data, size_t size) { link.feed(data, size); }
    bool poll() { return link.poll(); }
};

} // namespace hornet_native
