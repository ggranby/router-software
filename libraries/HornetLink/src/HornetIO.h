/**
 * @file HornetIO.h
 * @brief Where inputs and outputs are wired: direct pins, 74HC165 / 74HC595
 *        shift registers, switch matrices and CD4067 analog multiplexers.
 *
 * Every element takes a DigitalIn, AnalogIn or DigitalOut. Moving a switch
 * from a direct pin to a shift register is a one-word change:
 * @code
 *   Hornet::Switch masterArm(Hornet::MasterArm::MasterArm, Hornet::pin(4));
 *   Hornet::Switch masterArm(Hornet::MasterArm::MasterArm, shiftIn.bit(4));
 * @endcode
 * Contacts are active-low: wire the switch between the input and GND. Direct
 * pins get the internal pull-up automatically.
 *
 * Arduino-only header (uses digitalRead, analogRead, ...).
 */

#pragma once

#include <Arduino.h>

#include "HornetTypes.h"

// ADC resolution of analogRead() on this board (used to scale to 0-65535).
#ifndef HORNET_ADC_BITS
  #if defined(ESP32)
    #define HORNET_ADC_BITS 12
  #else
    #define HORNET_ADC_BITS 10
  #endif
#endif

namespace Hornet {

// ── Sources ──────────────────────────────────────────────────────────────

/// Something that can be read as numbered on/off contacts.
class DigitalSource {
public:
    /// Called once per loop before elements read. Shift registers etc. latch here.
    virtual void scan() {}
    /// Prepare contact @p index (e.g. set INPUT_PULLUP).
    virtual void configure(uint8_t) {}
    /// True if contact @p index is closed.
    virtual bool closed(uint8_t index) = 0;

    DigitalSource* nextSource = nullptr;
    static DigitalSource*& first() { static DigitalSource* head = nullptr; return head; }

protected:
    /// Sources that need scan() register themselves.
    void registerForScan() { nextSource = first(); first() = this; }
    ~DigitalSource() = default;
};

/// Something that can be read as numbered analog channels, scaled to 0-65535.
class AnalogSource {
public:
    virtual uint16_t read(uint8_t index) = 0;

protected:
    ~AnalogSource() = default;
};

/// Something that drives numbered on/off outputs.
class DigitalSink {
public:
    virtual void configure(uint8_t) {}
    virtual void set(uint8_t index, bool on) = 0;
    /// Called once per loop after elements wrote (shift registers latch here).
    virtual void flush() {}

    DigitalSink* nextSink = nullptr;
    static DigitalSink*& first() { static DigitalSink* head = nullptr; return head; }

protected:
    void registerForFlush() { nextSink = first(); first() = this; }
    ~DigitalSink() = default;
};

/// A single contact: a source plus a contact number. `none` means "not wired".
struct DigitalIn {
    DigitalSource* source;
    uint8_t index;
    bool wired() const { return source != nullptr; }
    bool closed() const { return source && source->closed(index); }
    void configure() const { if (source) source->configure(index); }
};

struct AnalogIn {
    AnalogSource* source;
    uint8_t index;
    uint16_t read() const { return source ? source->read(index) : 0; }
};

struct DigitalOut {
    DigitalSink* sink;
    uint8_t index;
    bool activeLow;
    void set(bool on) const { if (sink) sink->set(index, activeLow ? !on : on); }
    void configure() const { if (sink) sink->configure(index); }
    /// Same output, driven LOW for "on" (e.g. an LED wired from +5 V to the pin).
    DigitalOut inverted() const { return DigitalOut{sink, index, !activeLow}; }
};

/// Placeholder for "this position has no contact" (e.g. the centre of an ON-OFF-ON switch).
constexpr DigitalIn none = {nullptr, 0};

// ── Direct board pins ────────────────────────────────────────────────────

class DirectPins : public DigitalSource, public AnalogSource {
public:
    void configure(uint8_t pin) override { ::pinMode(pin, INPUT_PULLUP); }
    bool closed(uint8_t pin) override { return ::digitalRead(pin) == LOW; }
    uint16_t read(uint8_t pin) override {
        return static_cast<uint16_t>((static_cast<uint32_t>(::analogRead(pin)) * 65535UL) /
                                     ((1UL << HORNET_ADC_BITS) - 1UL));
    }

    static DirectPins& instance() { static DirectPins p; return p; }
};

/// A contact on a board pin (switch between the pin and GND).
inline DigitalIn pin(uint8_t p) { return DigitalIn{&DirectPins::instance(), p}; }
/// An analog board pin (A0, A1, ...).
inline AnalogIn analogPin(uint8_t p) { return AnalogIn{&DirectPins::instance(), p}; }

/// A board pin used as an output (LED to GND through a resistor).
class DirectOutputs : public DigitalSink {
public:
    void configure(uint8_t p) override { ::pinMode(p, OUTPUT); }
    void set(uint8_t p, bool on) override { ::digitalWrite(p, on ? HIGH : LOW); }
    static DirectOutputs& instance() { static DirectOutputs o; return o; }
};

inline DigitalOut output(uint8_t p) { return DigitalOut{&DirectOutputs::instance(), p, false}; }

// ── 74HC165 parallel-in shift registers (inputs) ─────────────────────────

/**
 * @brief Chain of up to 8 74HC165 chips (64 contacts).
 * bit(0) is the first bit shifted out (input H of the chip whose QH pin goes
 * to the Arduino); bit(8) is the first bit of the next chip in the chain.
 * Use the wiring test to confirm which bit a switch landed on.
 */
class Hc165 : public DigitalSource {
public:
    Hc165(uint8_t loadPin, uint8_t clockPin, uint8_t dataPin, uint8_t chips = 1)
        : load_(loadPin), clock_(clockPin), data_(dataPin), chips_(chips > 8 ? 8 : chips) {
        registerForScan();
    }
    DigitalIn bit(uint8_t n) { return DigitalIn{this, n}; }

    void configure(uint8_t) override {
        if (configured_) return;
        configured_ = true;
        ::pinMode(load_, OUTPUT);
        ::pinMode(clock_, OUTPUT);
        ::pinMode(data_, INPUT);
        ::digitalWrite(load_, HIGH);
    }
    void scan() override {
        if (!configured_) return;
        ::digitalWrite(load_, LOW);
        delayMicroseconds(5);
        ::digitalWrite(load_, HIGH);
        for (uint8_t i = 0; i < chips_ * 8; i++) {
            const bool level = ::digitalRead(data_) == HIGH;
            if (level) bits_[i >> 3] |= static_cast<uint8_t>(1u << (i & 7));
            else bits_[i >> 3] &= static_cast<uint8_t>(~(1u << (i & 7)));
            ::digitalWrite(clock_, HIGH);
            delayMicroseconds(1);
            ::digitalWrite(clock_, LOW);
        }
    }
    bool closed(uint8_t n) override {
        if (n >= chips_ * 8) return false;
        return (bits_[n >> 3] & (1u << (n & 7))) == 0; // active low: pulled up, switch to GND
    }

private:
    uint8_t load_, clock_, data_, chips_;
    bool configured_ = false;
    uint8_t bits_[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
};

// ── 74HC595 serial-in shift registers (outputs) ──────────────────────────

class Hc595 : public DigitalSink {
public:
    Hc595(uint8_t latchPin, uint8_t clockPin, uint8_t dataPin, uint8_t chips = 1)
        : latch_(latchPin), clock_(clockPin), data_(dataPin), chips_(chips > 8 ? 8 : chips) {
        registerForFlush();
    }
    DigitalOut bit(uint8_t n) { return DigitalOut{this, n, false}; }

    void configure(uint8_t) override {
        if (configured_) return;
        configured_ = true;
        ::pinMode(latch_, OUTPUT);
        ::pinMode(clock_, OUTPUT);
        ::pinMode(data_, OUTPUT);
        dirty_ = true;
    }
    void set(uint8_t n, bool on) override {
        if (n >= chips_ * 8) return;
        const uint8_t mask = static_cast<uint8_t>(1u << (n & 7));
        const uint8_t before = bits_[n >> 3];
        bits_[n >> 3] = on ? static_cast<uint8_t>(before | mask) : static_cast<uint8_t>(before & ~mask);
        if (bits_[n >> 3] != before) dirty_ = true;
    }
    void flush() override {
        if (!configured_ || !dirty_) return;
        dirty_ = false;
        ::digitalWrite(latch_, LOW);
        for (int8_t chip = static_cast<int8_t>(chips_ - 1); chip >= 0; chip--)
            ::shiftOut(data_, clock_, MSBFIRST, bits_[chip]);
        ::digitalWrite(latch_, HIGH);
    }

private:
    uint8_t latch_, clock_, data_, chips_;
    bool configured_ = false, dirty_ = false;
    uint8_t bits_[8] = {};
};

// ── Switch / key matrix ──────────────────────────────────────────────────

/**
 * @brief Row/column key matrix (up to 8 x 8), e.g. the UFC keypad.
 * Rows are driven LOW one at a time; columns are read with pull-ups.
 * Fit a diode per key if several keys may be held at once.
 * key(row, col) gives the contact.
 */
class Matrix : public DigitalSource {
public:
    template <size_t R, size_t C>
    Matrix(const uint8_t (&rowPins)[R], const uint8_t (&colPins)[C])
        : rows_(rowPins), cols_(colPins), nRows_(R > 8 ? 8 : R), nCols_(C > 8 ? 8 : C) {
        static_assert(R <= 8 && C <= 8, "Hornet::Matrix supports at most 8 rows and 8 columns");
        registerForScan();
    }
    DigitalIn key(uint8_t row, uint8_t col) { return DigitalIn{this, static_cast<uint8_t>(row * 8 + col)}; }

    void configure(uint8_t) override {
        if (configured_) return;
        configured_ = true;
        for (uint8_t r = 0; r < nRows_; r++) { ::pinMode(rows_[r], INPUT); }
        for (uint8_t c = 0; c < nCols_; c++) ::pinMode(cols_[c], INPUT_PULLUP);
    }
    void scan() override {
        if (!configured_) return;
        for (uint8_t r = 0; r < nRows_; r++) {
            ::pinMode(rows_[r], OUTPUT);
            ::digitalWrite(rows_[r], LOW);
            delayMicroseconds(5);
            uint8_t row = 0;
            for (uint8_t c = 0; c < nCols_; c++)
                if (::digitalRead(cols_[c]) == LOW) row |= static_cast<uint8_t>(1u << c);
            state_[r] = row;
            ::pinMode(rows_[r], INPUT); // high-impedance between scans
        }
    }
    bool closed(uint8_t n) override {
        const uint8_t r = n >> 3, c = n & 7;
        return r < nRows_ && c < nCols_ && (state_[r] & (1u << c));
    }

private:
    const uint8_t* rows_;
    const uint8_t* cols_;
    uint8_t nRows_, nCols_;
    bool configured_ = false;
    uint8_t state_[8] = {};
};

// ── CD4067 16-channel analog multiplexer ─────────────────────────────────

class Cd4067 : public AnalogSource, public DigitalSource {
public:
    Cd4067(uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t analogInputPin)
        : sig_(analogInputPin) {
        sel_[0] = s0; sel_[1] = s1; sel_[2] = s2; sel_[3] = s3;
    }
    AnalogIn channel(uint8_t ch) { return AnalogIn{this, ch}; }
    /// Read a switch through the mux (below 25 % of travel = closed).
    DigitalIn contact(uint8_t ch) { return DigitalIn{this, ch}; }

    uint16_t read(uint8_t ch) override {
        select(ch);
        return static_cast<uint16_t>((static_cast<uint32_t>(::analogRead(sig_)) * 65535UL) /
                                     ((1UL << HORNET_ADC_BITS) - 1UL));
    }
    void configure(uint8_t) override { select(0); }
    bool closed(uint8_t ch) override { return read(ch) < 16384; }

private:
    void select(uint8_t ch) {
        if (!configured_) {
            configured_ = true;
            for (uint8_t i = 0; i < 4; i++) ::pinMode(sel_[i], OUTPUT);
        }
        for (uint8_t i = 0; i < 4; i++) ::digitalWrite(sel_[i], (ch >> i) & 1 ? HIGH : LOW);
        delayMicroseconds(10); // let the mux and the ADC sample cap settle
    }
    uint8_t sel_[4];
    uint8_t sig_;
    bool configured_ = false;
};

} // namespace Hornet
