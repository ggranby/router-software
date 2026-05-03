/**
 * @file HornetLinkMode.h
 * @brief Operating-mode frame handling for Hornet Link devices.
 *
 * The PC hub can push one of three operating modes to all devices:
 *  - 0 = Sim         (live DCS data stream)
 *  - 1 = Preflight   (BIT / lamp test state)
 *  - 2 = Maintenance (panel self-check, no DCS required)
 *
 * The bridge sends:  AA DE AD 04 [mode]
 * The device replies: AA DE AD 05 [local_mode]
 */

#pragma once
#include "HornetLinkBase.h"

enum class HornetLinkMode : uint8_t {
    Sim         = 0,
    Preflight   = 1,
    Maintenance = 2,
};

/**
 * @brief Mixin / helper for mode-frame parsing and response.
 *
 * Devices inherit or embed this and call processModeByte() in
 * their receive loop.
 */
class HornetLinkModeHandler {
public:
    HornetLinkMode currentMode() const { return mode_; }

    /**
     * @brief Feed one incoming byte.  Returns true when a complete,
     *        valid mode frame has been received and applied.
     *
     * @param b    Byte from the serial stream.
     * @param out  Stream to write the mode-ack response on.
     */
    bool processModeByte(uint8_t b, Stream& out) {
        switch (state_) {
        case S::H0: state_ = (b == kHL_HDR0) ? S::H1 : S::H0; break;
        case S::H1: state_ = (b == kHL_HDR1) ? S::H2 : S::H0; break;
        case S::H2: state_ = (b == kHL_HDR2) ? S::FT : S::H0; break;
        case S::FT:
            if (b == kHL_MODE) { state_ = S::MV; }
            else               { state_ = S::H0; }
            break;
        case S::MV:
            state_ = S::H0;
            if (b <= 2) {
                mode_ = static_cast<HornetLinkMode>(b);
                // Send ack: AA DE AD 05 [local_mode]
                out.write(kHL_HDR0); out.write(kHL_HDR1); out.write(kHL_HDR2);
                out.write(kHL_MACK);
                out.write(static_cast<uint8_t>(mode_));
                onModeChange(mode_);
                return true;
            }
            break;
        }
        return false;
    }

protected:
    /** Override to react when the mode changes. */
    virtual void onModeChange(HornetLinkMode newMode) {}

private:
    enum class S { H0, H1, H2, FT, MV };
    S               state_ = S::H0;
    HornetLinkMode  mode_  = HornetLinkMode::Sim;
};
