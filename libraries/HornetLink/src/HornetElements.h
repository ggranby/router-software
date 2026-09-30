/**
 * @file HornetElements.h
 * @brief Declarative cockpit elements: describe what is wired where, the
 *        library does the rest (debounce, filtering, sync, lamp test).
 *
 * @code
 *   #include <Hornet.h>
 *   using namespace Hornet;
 *
 *   Switch masterArm(MasterArm::MasterArm, 4);          // pin 4 closed = ARM
 *   Button aa(MasterArm::ModeAa, 5);
 *   Lamp   aaLight(MasterArm::AaLt, output(9));
 *   Panel  panel("MASTER ARM");
 *
 *   void setup() { panel.beginUsb(Serial); }
 *   void loop()  { panel.update(); }
 * @endcode
 *
 * Every constructor checks at compile time that the element fits the control
 * (a Pot cannot be bound to a switch, a 3-position switch needs 3 inputs...).
 * Arduino-only header.
 */

#pragma once

#include "HornetFilters.h"
#include "HornetIO.h"
#include "protocol/HnRecords.h"

namespace Hornet {

// Contacts can be given as a pin number or as a DigitalIn (shiftIn.bit(3), none, ...).
inline DigitalIn toIn(DigitalIn d) { return d; }
inline DigitalIn toIn(int boardPin) { return pin(static_cast<uint8_t>(boardPin)); }

constexpr uint8_t kRoleIn = 1;
constexpr uint8_t kRoleOut = 2;

/// Base of every element. Elements link themselves into a list; no heap is used.
class Element {
public:
    uint16_t id() const { return id_; }
    uint8_t roles() const { return roles_; }
    Element* next() const { return next_; }
    static Element*& first() { static Element* head = nullptr; return head; }

    virtual void begin(uint32_t) {}
    virtual void poll(uint32_t) {}
    virtual void onState(const hn::StateRecord&) {}
    virtual void onMode(uint8_t) {}
    /// Physical position for sync reports. False if the element has none (encoder).
    virtual bool syncValue(uint16_t&) const { return false; }
    /// Set the raw end stops (CONFIG CALIBRATION). False if the element has none.
    virtual bool calibrate(uint16_t, uint16_t) { return false; }

    /// Where input events go (set by the Panel).
    typedef void (*InputSink)(uint16_t id, uint8_t action, uint16_t arg);
    static InputSink& sink() { static InputSink s = nullptr; return s; }

protected:
    Element(uint16_t id, uint8_t roles) : id_(id), roles_(roles) {
        // Append so iteration follows declaration order (nicer wiring-test output).
        Element** p = &first();
        while (*p) p = &(*p)->next_;
        *p = this;
    }
    ~Element() = default;
    void emit(uint8_t action, uint16_t arg) const { if (sink()) sink()(id_, action, arg); }
    void addRole(uint8_t r) { roles_ = static_cast<uint8_t>(roles_ | r); }

private:
    uint16_t id_;
    uint8_t roles_;
    Element* next_ = nullptr;
};

// ── Positional inputs (switches, selectors) ──────────────────────────────

namespace detail {
/// Shared logic: one contact per position, debounce, report SET_POSITION.
class Positional : public Element {
public:
    static constexpr uint8_t kMaxPositions = 12;
    Debouncer debounce;

    void begin(uint32_t) override {
        for (uint8_t i = 0; i < count_; i++) inputs_[i].configure();
    }
    void poll(uint32_t now) override {
        const uint8_t raw = readRaw();
        if (!primed_) { debounce.reset(raw, now); primed_ = true; return; }
        if (debounce.update(raw, now)) emit(hn::ACTION_SET_POSITION, debounce.position());
    }
    bool syncValue(uint16_t& v) const override { v = debounce.position() == 0xFF ? 0 : debounce.position(); return primed_; }

protected:
    Positional(uint16_t id, const DigitalIn* in, uint8_t n) : Element(id, kRoleIn), count_(n) {
        for (uint8_t i = 0; i < n && i < kMaxPositions; i++) inputs_[i] = in[i];
    }
    uint8_t readRaw() const {
        uint8_t open = 0xFF;
        for (uint8_t i = 0; i < count_; i++) {
            if (!inputs_[i].wired()) { if (open == 0xFF) open = i; continue; }
            if (inputs_[i].closed()) return i;
        }
        // Nothing closed: the unwired position, or stay put (between detents).
        return open != 0xFF ? open : (debounce.position() == 0xFF ? 0 : debounce.position());
    }
    DigitalIn inputs_[kMaxPositions];
    uint8_t count_;
    bool primed_ = false;
};
} // namespace detail

/**
 * @brief 2- or 3-position toggle switch (including guards and magnetically held switches).
 *
 * Give one contact per position, in the order of the control reference, using
 * Hornet::none for the position without a contact:
 * @code
 *   Switch apu(Apu::ApuControl, none, 7);               // OFF has no contact, ON = pin 7
 *   Switch crank(Apu::EngineCrank, 4, none, 5);         // LEFT-OFF-RIGHT
 *   Switch masterArm(MasterArm::MasterArm, 4);          // 2-position shortcut: pin = 2nd position
 * @endcode
 */
class Switch : public detail::Positional {
public:
    template <Kind K, uint8_t N, typename... In>
    Switch(const Control<K, N>& c, In... contacts) : Positional(c.id, Expand<N>(toIn(contacts)...).in, N) {
        static_assert(K == Kind::Switch,
            "Hornet::Switch needs a switch control. Use Hornet::Button for pushbuttons, "
            "Hornet::Selector for rotary selectors (see docs/F18C_CONTROL_REFERENCE.md).");
        static_assert(sizeof...(In) == N || (N == 2 && sizeof...(In) == 1),
            "Hornet::Switch: give one contact per position in the order of the control reference "
            "(Hornet::none for the position with no contact), or a single contact for a 2-position switch.");
    }

    /// Drive a hold coil (magnetically held switch). The coil is on while DCS
    /// reports @p heldPosition, so the switch drops back when the sim releases it.
    Switch& holdCoil(DigitalOut coil, uint8_t heldPosition) {
        coil_ = coil;
        held_ = heldPosition;
        addRole(kRoleOut);
        return *this;
    }
    void begin(uint32_t now) override {
        Positional::begin(now);
        coil_.configure();
    }
    void onState(const hn::StateRecord& r) override {
        if (coil_.sink && r.id == id()) coil_.set(r.value == held_);
    }

private:
    template <uint8_t N>
    struct Expand {
        DigitalIn in[N];
        explicit Expand(DigitalIn only) : in() { in[0] = none; in[1] = only; for (uint8_t i = 2; i < N; i++) in[i] = none; }
        template <typename... D>
        explicit Expand(DigitalIn a, DigitalIn b, D... rest) : in{a, b, rest...} {}
    };
    DigitalOut coil_ = {nullptr, 0, false};
    uint8_t held_ = 0;
};

/**
 * @brief Rotary selector: one contact per position (Hornet::none for one
 * position without a contact), or a resistor ladder on one analog input.
 */
class Selector : public detail::Positional {
public:
    template <Kind K, uint8_t N, typename... In>
    Selector(const Control<K, N>& c, In... contacts) : Positional(c.id, Arr<sizeof...(In)>{{toIn(contacts)...}}.in, N) {
        static_assert(K == Kind::Selector || K == Kind::Switch,
            "Hornet::Selector needs a selector (or switch) control. Use Hornet::Button for pushbuttons.");
        static_assert(sizeof...(In) == N,
            "Hornet::Selector: give exactly one contact per position, in the order of the control reference "
            "(Hornet::none for the position with no contact).");
        static_assert(N <= kMaxPositions, "Hornet::Selector supports at most 12 positions");
    }

    /// Resistor-ladder selector on one analog input: position = reading split into N equal bands.
    template <Kind K, uint8_t N>
    Selector(const Control<K, N>& c, AnalogIn ladder) : Positional(c.id, nullptr, 0), ladder_(ladder), bands_(N) {
        static_assert(K == Kind::Selector || K == Kind::Switch, "Hornet::Selector needs a selector (or switch) control.");
    }

    void poll(uint32_t now) override {
        if (!bands_) { Positional::poll(now); return; }
        const uint32_t v = ladder_.read();
        uint8_t pos = static_cast<uint8_t>((v * bands_) >> 16);
        if (pos >= bands_) pos = static_cast<uint8_t>(bands_ - 1);
        if (!primed_) { debounce.reset(pos, now); primed_ = true; return; }
        if (debounce.update(pos, now)) emit(hn::ACTION_SET_POSITION, pos);
    }

private:
    template <size_t M>
    struct Arr { DigitalIn in[M]; };
    AnalogIn ladder_ = {nullptr, 0};
    uint8_t bands_ = 0;
};

/// Momentary pushbutton. Sends PRESS / RELEASE.
class Button : public Element {
public:
    template <Kind K, uint8_t N, typename In>
    Button(const Control<K, N>& c, In contact) : Element(c.id, kRoleIn), in_(toIn(contact)) {
        static_assert(K == Kind::Button,
            "Hornet::Button needs a pushbutton control. Use Hornet::Switch for toggle switches.");
    }
    Debouncer debounce;

    void begin(uint32_t) override { in_.configure(); }
    void poll(uint32_t now) override {
        const uint8_t raw = in_.closed() ? 1 : 0;
        if (!primed_) { debounce.reset(raw, now); primed_ = true; return; }
        if (debounce.update(raw, now)) emit(raw ? hn::ACTION_PRESS : hn::ACTION_RELEASE, raw);
    }
    bool syncValue(uint16_t& v) const override { v = debounce.position() == 1 ? 1 : 0; return primed_; }

private:
    DigitalIn in_;
    bool primed_ = false;
};

/// Potentiometer (absolute axis). Smoothing, deadband and rate limit are on by default.
class Pot : public Element {
public:
    template <Kind K, uint8_t N>
    Pot(const Control<K, N>& c, AnalogIn in) : Element(c.id, kRoleIn), in_(in) {
        static_assert(K == Kind::Axis,
            "Hornet::Pot needs an axis control (a knob with a continuous range). "
            "For stepped knobs use Hornet::Selector or Hornet::Encoder.");
    }
    template <Kind K, uint8_t N>
    Pot(const Control<K, N>& c, int analogBoardPin) : Pot(c, analogPin(static_cast<uint8_t>(analogBoardPin))) {}

    PotFilter filter;

    /// Readings (0-65535 scale) at the physical end stops. Use the wiring test to find them.
    Pot& range(uint16_t rawMin, uint16_t rawMax) { min_ = rawMin; max_ = rawMax; return *this; }
    bool calibrate(uint16_t rawMin, uint16_t rawMax) override {
        if (rawMax <= rawMin) return false;
        range(rawMin, rawMax);
        return true;
    }
    Pot& reverse(bool r = true) { reverse_ = r; return *this; }

    void poll(uint32_t now) override {
        uint16_t v = scaleToAxis(in_.read(), min_, max_);
        if (reverse_) v = static_cast<uint16_t>(65535u - v);
        if (!filter.primed()) { filter.prime(v); return; }
        uint16_t out;
        if (filter.update(v, now, out)) emit(hn::ACTION_ANALOG, out);
    }
    bool syncValue(uint16_t& v) const override { v = filter.value(); return true; }

private:
    AnalogIn in_;
    uint16_t min_ = 0, max_ = 65535;
    bool reverse_ = false;
};

/**
 * @brief Rotary encoder. Works with rotary controls (channel knobs), selectors
 * (steps through the positions) and axes (each detent moves the axis by axisStep).
 */
class Encoder : public Element {
public:
    template <Kind K, uint8_t N, typename A, typename B>
    Encoder(const Control<K, N>& c, A a, B b) : Element(c.id, kRoleIn), a_(toIn(a)), b_(toIn(b)), isAxis_(K == Kind::Axis) {
        static_assert(K == Kind::Rotary || K == Kind::Selector || K == Kind::Axis,
            "Hornet::Encoder needs a rotary, selector or axis control.");
    }
    QuadratureDecoder decoder;

    Encoder& detents(uint8_t transitionsPerDetent) { decoder.transitionsPerDetent = transitionsPerDetent; return *this; }
    Encoder& accelerate(bool on = true) { decoder.acceleration = on; return *this; }
    Encoder& reverse(bool r = true) { reverse_ = r; return *this; }
    /// Axis units per detent when bound to an axis (default 1/32 of full travel).
    Encoder& axisStep(uint16_t units) { axisStep_ = units; return *this; }

    void begin(uint32_t) override { a_.configure(); b_.configure(); decoder.reset(a_.closed(), b_.closed()); }
    void poll(uint32_t now) override {
        int8_t s = decoder.update(a_.closed(), b_.closed(), now);
        if (reverse_) s = static_cast<int8_t>(-s);
        pending_ = static_cast<int16_t>(pending_ + s);
        if (pending_ && (now - lastSent_) >= 20) {
            const int32_t amount = isAxis_ ? static_cast<int32_t>(pending_) * axisStep_ : pending_;
            const int16_t clamped = static_cast<int16_t>(amount > 32767 ? 32767 : (amount < -32767 ? -32767 : amount));
            emit(hn::ACTION_STEP, static_cast<uint16_t>(clamped));
            pending_ = 0;
            lastSent_ = now;
        }
    }

private:
    DigitalIn a_, b_;
    bool isAxis_;
    bool reverse_ = false;
    int16_t pending_ = 0;
    uint16_t axisStep_ = 2048;
    uint32_t lastSent_ = 0;
};

// ── Outputs ──────────────────────────────────────────────────────────────

/// On/off light driven by DCS. Lights up in lamp test.
class Lamp : public Element {
public:
    typedef void (*Handler)(bool on);

    template <Kind K, uint8_t N>
    Lamp(const Control<K, N>& c, DigitalOut out) : Element(c.id, kRoleOut), out_(out) {
        static_assert(K == Kind::Lamp, "Hornet::Lamp needs a lamp (indicator light) control.");
    }
    /// Custom driver (e.g. an addressable LED): called with true/false.
    template <Kind K, uint8_t N>
    Lamp(const Control<K, N>& c, Handler fn) : Element(c.id, kRoleOut), out_{nullptr, 0, false}, fn_(fn) {
        static_assert(K == Kind::Lamp, "Hornet::Lamp needs a lamp (indicator light) control.");
    }

    void begin(uint32_t) override { out_.configure(); apply(); }
    void onState(const hn::StateRecord& r) override { on_ = r.value != 0; apply(); }
    void onMode(uint8_t mode) override { test_ = mode == hn::MODE_LAMP_TEST; apply(); }
    bool isOn() const { return on_; }

private:
    void apply() {
        const bool v = on_ || test_;
        out_.set(v);
        if (fn_) fn_(v);
    }
    DigitalOut out_;
    Handler fn_ = nullptr;
    bool on_ = false, test_ = false;
};

/**
 * @brief Analog output: needle gauges (servo, stepper, air-core) or PWM
 * backlight. The DCS value 0-65535 goes through an optional calibration
 * curve, then to your handler or a PWM pin.
 */
class Gauge : public Element {
public:
    typedef void (*Handler)(int32_t value);

    template <Kind K, uint8_t N>
    Gauge(const Control<K, N>& c, Handler fn, const CurvePoint* curve = nullptr, uint8_t points = 0)
        : Element(c.id, kRoleOut), fn_(fn), curve_(curve), points_(points) {
        static_assert(K == Kind::Gauge || K == Kind::Axis,
            "Hornet::Gauge needs a gauge (or axis) control.");
    }
    /// PWM pin (backlight, analog meter): 0-65535 becomes analogWrite 0-255.
    template <Kind K, uint8_t N>
    Gauge(const Control<K, N>& c, uint8_t pwmPin) : Element(c.id, kRoleOut), pwmPin_(pwmPin) {
        static_assert(K == Kind::Gauge || K == Kind::Axis,
            "Hornet::Gauge needs a gauge (or axis) control.");
    }

    void begin(uint32_t) override { if (pwmPin_ != 0xFF) ::pinMode(pwmPin_, OUTPUT); }
    void onState(const hn::StateRecord& r) override { value_ = r.value; apply(); }
    void onMode(uint8_t mode) override { test_ = mode == hn::MODE_LAMP_TEST; apply(); }
    uint16_t value() const { return value_; }

private:
    void apply() {
        const uint16_t in = test_ ? 65535 : value_;
        if (pwmPin_ != 0xFF) ::analogWrite(pwmPin_, in >> 8);
        if (fn_) fn_(applyCurve(curve_, points_, in));
    }
    Handler fn_ = nullptr;
    const CurvePoint* curve_ = nullptr;
    uint8_t points_ = 0;
    uint8_t pwmPin_ = 0xFF;
    uint16_t value_ = 0;
    bool test_ = false;
};

/// Fixed-length text (UFC, IFEI...). Your handler draws it (7-segment, OLED, LCD).
class TextDisplay : public Element {
public:
    typedef void (*Handler)(const char* text, uint8_t length);

    template <Kind K, uint8_t N>
    TextDisplay(const Control<K, N>& c, Handler fn) : Element(c.id, kRoleOut), fn_(fn), len_(N) {
        static_assert(K == Kind::Text, "Hornet::TextDisplay needs a text control.");
        static_assert(N <= 32, "text controls longer than 32 characters are not supported");
        memset(value_, ' ', len_);
        value_[len_] = '\0';
    }

    void onState(const hn::StateRecord& r) override {
        if (!fn_ || r.kind != hn::VALUE_TEXT) return;
        const uint8_t n = r.textLen < len_ ? r.textLen : len_;
        memcpy(value_, r.text, n);
        for (uint8_t i = n; i < len_; i++) value_[i] = ' ';
        value_[len_] = '\0';
        if (!test_) fn_(value_, len_);
    }
    void onMode(uint8_t mode) override {
        const bool t = mode == hn::MODE_LAMP_TEST;
        if (t == test_ || !fn_) return;
        test_ = t;
        if (t) {
            char buf[33];
            memset(buf, '8', len_);
            buf[len_] = '\0';
            fn_(buf, len_);
        } else {
            fn_(value_, len_);
        }
    }

private:
    Handler fn_;
    uint8_t len_;
    char value_[33] = {};
    bool test_ = false;
};

} // namespace Hornet
