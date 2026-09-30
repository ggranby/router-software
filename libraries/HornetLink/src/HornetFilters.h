/**
 * @file HornetFilters.h
 * @brief Input conditioning used by the panel elements: debounce, pot
 *        smoothing, encoder decoding, calibration curves.
 *
 * Plain C++11 with no Arduino dependency so the behaviour is unit-tested on
 * the PC (tests/test_hornet_v2.cpp). Every time value is in milliseconds and
 * is passed in by the caller.
 */

#pragma once

#include <stdint.h>

namespace Hornet {

/**
 * @brief Debounces a position reading (switch or selector).
 * A new position is accepted once it has been read unchanged for @c ms.
 */
class Debouncer {
public:
    explicit Debouncer(uint8_t ms = 10) : ms_(ms) {}
    void setTime(uint8_t ms) { ms_ = ms; }

    /// Feed the raw reading. Returns true when the stable position changes.
    bool update(uint8_t raw, uint32_t nowMs) {
        if (raw != candidate_) { candidate_ = raw; since_ = nowMs; return false; }
        if (raw == stable_ || (nowMs - since_) < ms_) return false;
        stable_ = raw;
        return true;
    }
    /// Accept a reading immediately (used at start-up).
    void reset(uint8_t raw, uint32_t nowMs) { stable_ = candidate_ = raw; since_ = nowMs; }
    uint8_t position() const { return stable_; }

private:
    uint8_t ms_;
    uint8_t stable_ = 0xFF;
    uint8_t candidate_ = 0xFF;
    uint32_t since_ = 0;
};

/**
 * @brief Smooths a potentiometer and decides when a new value is worth sending.
 *
 * - Exponential smoothing (strength 0-7; each step halves the noise).
 * - Deadband: changes smaller than @c deadband (0-65535 units) are ignored.
 * - Rate limit: at most one report per @c minIntervalMs.
 * - Settle: once the pot stops moving, the exact resting value is sent once,
 *   so the sim ends up where the knob is even with a large deadband.
 */
class PotFilter {
public:
    uint8_t smoothing = 3;          ///< 0 = off, 3 = default, 7 = heavy
    uint16_t deadband = 512;        ///< ~0.8 % of full travel
    uint16_t minIntervalMs = 20;    ///< max ~50 reports per second
    uint16_t settleMs = 150;

    /// @param raw16 reading scaled to 0-65535. Returns true and sets @p out when a report is due.
    bool update(uint16_t raw16, uint32_t nowMs, uint16_t& out) {
        const uint32_t target = static_cast<uint32_t>(raw16) << 8;
        if (!primed_) { acc_ = target; primed_ = true; }
        else if (smoothing == 0) acc_ = target;
        else acc_ = acc_ - (acc_ >> smoothing) + (target >> smoothing);
        const uint16_t v = static_cast<uint16_t>(acc_ >> 8);

        const uint16_t diff = v > sent_ ? static_cast<uint16_t>(v - sent_) : static_cast<uint16_t>(sent_ - v);
        if (!haveSent_ || diff >= deadband) {
            if (haveSent_ && (nowMs - lastSent_) < minIntervalMs) return false; // retried next call
            moving_ = true;
            return report(v, nowMs, out);
        }
        if (moving_ && (nowMs - lastSent_) >= settleMs) {
            moving_ = false;
            if (diff != 0) return report(v, nowMs, out);
        }
        return false;
    }

    /// Last value reported (for sync reports).
    uint16_t value() const { return sent_; }

private:
    bool report(uint16_t v, uint32_t nowMs, uint16_t& out) {
        sent_ = v;
        haveSent_ = true;
        lastSent_ = nowMs;
        out = v;
        return true;
    }
    uint32_t acc_ = 0;
    bool primed_ = false;
    bool haveSent_ = false;
    bool moving_ = false;
    uint16_t sent_ = 0;
    uint32_t lastSent_ = 0;
};

/**
 * @brief Quadrature decoder for mechanical rotary encoders.
 *
 * Counts valid Gray-code transitions and emits one step per detent
 * (@c transitionsPerDetent, 4 for most panel encoders, 2 or 1 for some).
 * Invalid jumps (contact bounce) are ignored. Optional acceleration
 * multiplies steps when the knob is spun quickly.
 */
class QuadratureDecoder {
public:
    uint8_t transitionsPerDetent = 4;
    bool acceleration = false;
    uint8_t accelMs = 40;      ///< detents closer together than this count double
    uint8_t accelMax = 4;      ///< largest multiplier

    /// Feed the two contact states. Returns signed detent steps (usually 0).
    int8_t update(bool a, bool b, uint32_t nowMs) {
        static const int8_t kTable[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
        const uint8_t cur = static_cast<uint8_t>((a ? 2 : 0) | (b ? 1 : 0));
        acc_ = static_cast<int8_t>(acc_ + kTable[(prev_ << 2) | cur]);
        prev_ = cur;
        const int8_t per = static_cast<int8_t>(transitionsPerDetent ? transitionsPerDetent : 4);
        int8_t steps = 0;
        if (acc_ >= per) { steps = 1; acc_ = static_cast<int8_t>(acc_ - per); }
        else if (acc_ <= -per) { steps = -1; acc_ = static_cast<int8_t>(acc_ + per); }
        if (steps && acceleration) {
            if (steps == lastDir_ && (nowMs - lastStep_) < accelMs) {
                if (mult_ < accelMax) mult_++;
            } else {
                mult_ = 1;
            }
            lastStep_ = nowMs;
            lastDir_ = steps;
            steps = static_cast<int8_t>(steps * mult_);
        }
        return steps;
    }
    void reset(bool a, bool b) { prev_ = static_cast<uint8_t>((a ? 2 : 0) | (b ? 1 : 0)); acc_ = 0; }

private:
    uint8_t prev_ = 0;
    int8_t acc_ = 0;
    int8_t lastDir_ = 0;
    uint8_t mult_ = 1;
    uint32_t lastStep_ = 0;
};

/// One point of a calibration curve: sim value (0-65535) -> device value.
struct CurvePoint {
    uint16_t in;
    int32_t out;
};

/**
 * @brief Piecewise-linear lookup. @p points must be sorted by @c in.
 * Values outside the table are clamped to the first/last point.
 */
inline int32_t applyCurve(const CurvePoint* points, uint8_t count, uint16_t in) {
    if (!points || count == 0) return in;
    if (in <= points[0].in) return points[0].out;
    for (uint8_t i = 1; i < count; i++) {
        if (in <= points[i].in) {
            const CurvePoint& a = points[i - 1];
            const CurvePoint& b = points[i];
            const int32_t span = static_cast<int32_t>(b.in) - a.in;
            if (span <= 0) return b.out;
            return a.out + static_cast<int32_t>((static_cast<int64_t>(b.out - a.out) * (in - a.in)) / span);
        }
    }
    return points[count - 1].out;
}

/// Map a raw ADC range (after calibration) to 0-65535, clamped.
inline uint16_t scaleToAxis(uint16_t raw, uint16_t rawMin, uint16_t rawMax) {
    if (rawMax <= rawMin) return raw;
    if (raw <= rawMin) return 0;
    if (raw >= rawMax) return 65535;
    return static_cast<uint16_t>((static_cast<uint32_t>(raw - rawMin) * 65535u) / (rawMax - rawMin));
}

} // namespace Hornet
