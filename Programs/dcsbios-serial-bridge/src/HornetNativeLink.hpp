#pragma once
/**
 * @file HornetNativeLink.hpp
 * @brief Transport-neutral v2 HELLO negotiation around LinkSession.
 */

#include "HornetNative.hpp"

#include <chrono>
#include <functional>

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

} // namespace hornet_native
