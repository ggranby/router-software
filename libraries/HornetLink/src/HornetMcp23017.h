/**
 * @file HornetMcp23017.h
 * @brief MCP23017 16-bit I2C I/O expander as an input source.
 *
 * @code
 *   #include <Hornet.h>
 *   #include <HornetMcp23017.h>
 *   Hornet::Mcp23017 expander(0x20);                 // A0-A2 to GND
 *   Hornet::Switch masterArm(Hornet::MasterArm::MasterArm, expander.pin(3));  // GPA3
 *   void setup() { Wire.begin(); panel.beginUsb(Serial); }
 * @endcode
 * pin(0)-pin(7) are GPA0-GPA7, pin(8)-pin(15) are GPB0-GPB7. Internal pull-ups
 * are enabled; wire each switch to GND. Call Wire.begin() before panel.begin*().
 */

#pragma once

#include <Wire.h>

#include "HornetIO.h"

namespace Hornet {

class Mcp23017 : public DigitalSource {
public:
    explicit Mcp23017(uint8_t i2cAddress = 0x20, TwoWire& wire = Wire) : addr_(i2cAddress), wire_(wire) { registerForScan(); }
    DigitalIn pin(uint8_t n) { return DigitalIn{this, n}; }

    void configure(uint8_t) override {
        if (configured_) return;
        configured_ = true;
        write16(0x00, 0xFFFF); // IODIRA/B: all inputs
        write16(0x0C, 0xFFFF); // GPPUA/B: pull-ups on
    }
    void scan() override {
        if (!configured_) return;
        wire_.beginTransmission(addr_);
        wire_.write(static_cast<uint8_t>(0x12)); // GPIOA
        if (wire_.endTransmission() != 0) return;
        if (wire_.requestFrom(static_cast<int>(addr_), 2) != 2) return;
        const uint8_t a = static_cast<uint8_t>(wire_.read());
        const uint8_t b = static_cast<uint8_t>(wire_.read());
        bits_ = static_cast<uint16_t>(a | (b << 8));
    }
    bool closed(uint8_t n) override { return n < 16 && (bits_ & (1u << n)) == 0; }

private:
    void write16(uint8_t reg, uint16_t v) {
        wire_.beginTransmission(addr_);
        wire_.write(reg);
        wire_.write(static_cast<uint8_t>(v & 0xFF));
        wire_.write(static_cast<uint8_t>(v >> 8));
        wire_.endTransmission();
    }
    uint8_t addr_;
    TwoWire& wire_;
    bool configured_ = false;
    uint16_t bits_ = 0xFFFF;
};

} // namespace Hornet
