/**
 * @file HornetPanel.h
 * @brief Hornet::Panel (one panel board) and Hornet::BusMaster (RS-485 relay).
 *
 * The same sketch runs over USB or as an RS-485 slave; only the begin call changes:
 * @code
 *   panel.beginUsb(Serial);                         // plugged into the PC
 *   panel.beginRs485(Serial1, 12, 2);               // bus address 12, driver-enable pin 2
 *   panel.beginDebug(Serial);                       // plain-text mode for the Serial Monitor
 * @endcode
 * Arduino-only header.
 */

#pragma once

#include "HornetCatalog.h"
#include "HornetElements.h"
#include "protocol/HnMaster.h"

#if defined(ESP32)
  #include <Esp.h>
#endif
#if defined(__AVR__)
  #include <EEPROM.h>
#endif

#if defined(__AVR__)
extern int __heap_start, *__brkval; // avr-libc heap bounds, used for free-RAM reports
#endif

#ifndef HORNET_FW_MAJOR
  #define HORNET_FW_MAJOR 2
#endif
#ifndef HORNET_FW_MINOR
  #define HORNET_FW_MINOR 0
#endif

namespace Hornet {

/// Adapts an Arduino serial port (USB CDC, HardwareSerial) to the protocol engine.
class StreamPort : public hn::Port {
public:
    void attach(Stream* s, int8_t dePin) {
        stream_ = s;
        de_ = dePin;
        if (de_ >= 0) { ::pinMode(de_, OUTPUT); ::digitalWrite(de_, LOW); }
    }
    int read() override { return stream_ ? stream_->read() : -1; }
    void write(const uint8_t* d, size_t n) override { if (stream_) stream_->write(d, n); }
    void beginTransmit() override { if (de_ >= 0) ::digitalWrite(de_, HIGH); }
    void endTransmit() override {
        if (de_ < 0 || !stream_) return;
        stream_->flush(); // waits until the last byte has left the UART
        ::digitalWrite(de_, LOW);
    }
    Stream* stream() const { return stream_; }

private:
    Stream* stream_ = nullptr;
    int8_t de_ = -1;
};

namespace detail {
inline uint16_t freeRam() {
#if defined(__AVR__)
    int v;
    return static_cast<uint16_t>(reinterpret_cast<int>(&v) -
        (__brkval == 0 ? reinterpret_cast<int>(&__heap_start) : reinterpret_cast<int>(__brkval)));
#elif defined(ESP32)
    const uint32_t f = ESP.getFreeHeap();
    return static_cast<uint16_t>(f > 65535 ? 65535 : f);
#else
    return 0;
#endif
}

/// FNV-1a over a string, used for a default board id.
inline uint32_t fnv(const char* s, uint32_t h = 2166136261UL) {
    while (s && *s) { h ^= static_cast<uint8_t>(*s++); h *= 16777619UL; }
    return h;
}
} // namespace detail

/**
 * @brief One panel board. Declare it after your elements, call a begin
 * function in setup() and update() in loop().
 */
class Panel : public hn::NodeHandler {
public:
    typedef bool (*SaveHook)();
    typedef void (*ModeHook)(uint8_t mode);
    typedef void (*AddressHook)(uint8_t address);

    explicit Panel(const char* name) : name_(name), node_(port_, *this) {
        uint32_t h = detail::fnv(name);
        for (uint8_t i = 0; i < 4; i++) boardId_[i] = static_cast<uint8_t>(h >> (8 * i));
    }

    /// Plugged into the PC over USB (protocol v2 frames).
    template <class S>
    void beginUsb(S& serial, uint32_t baud = 250000) {
        serial.begin(baud);
        port_.attach(&serial, -1);
        loadBoardId();
        start();
        node_.beginUsb();
    }

    /// RS-485 slave at a fixed bus address (1-239). @p dePin drives DE and /RE of the transceiver.
    template <class S>
    void beginRs485(S& serial, uint8_t busAddress, int8_t dePin, uint32_t baud = 250000) {
        serial.begin(baud);
        port_.attach(&serial, dePin);
        loadBoardId();
        start();
        node_.seedRandom(identityHash());
        node_.beginBus(busAddress);
    }

    /**
     * @brief Plain-text mode for the Arduino Serial Monitor (no bridge needed).
     * Prints "MASTER_ARM.MASTER_ARM=ARM" when you move a control. Type
     * "MASTER_ARM.READY_LT=1" to drive an output, "SYNC" to list every input,
     * "MODE LAMP_TEST" / "MODE SIM" / "MODE MAINTENANCE" / "MODE WIRING_TEST" to change mode.
     */
    template <class S>
    void beginDebug(S& serial, uint32_t baud = 115200) {
        serial.begin(baud);
        debug_ = &serial;
        start();
        debug_->println(F("Hornet Link debug mode. Commands: NAME=VALUE, SYNC, MODE <SIM|LAMP_TEST|MAINTENANCE|WIRING_TEST>"));
    }

    /// Call every loop(). Keep loop() free of long delay() calls.
    void update() {
        const uint32_t now = millis();
        for (DigitalSource* s = DigitalSource::first(); s; s = s->nextSource) s->scan();
        for (Element* e = Element::first(); e; e = e->next()) e->poll(now);
        if (debug_) readDebug();
        else node_.update(now);
        for (DigitalSink* s = DigitalSink::first(); s; s = s->nextSink) s->flush();
    }

    /// Set a stable per-device id; otherwise hardware or EEPROM identity is used.
    void setBoardId(const uint8_t id[8]) { memcpy(boardId_, id, 8); boardIdSet_ = true; }
    /// Called when the bridge asks to persist settings (CONFIG SAVE). Return true on success.
    void onSave(SaveHook fn) { save_ = fn; }
    void onModeChange(ModeHook fn) { modeHook_ = fn; }
    /// Called after the bridge changed the bus address; store it in EEPROM if you want it to persist.
    void onAddressChange(AddressHook fn) { addrHook_ = fn; }

    uint8_t mode() const { return mode_; }
    const hn::Node& node() const { return node_; }

    // ── NodeHandler ──────────────────────────────────────────────────────
    void fillHello(hn::Hello& h) override {
        h.fwMajor = HORNET_FW_MAJOR;
        h.fwMinor = HORNET_FW_MINOR;
        h.catalogHash = kCatalogHash;
        memcpy(h.boardId, boardId_, 8);
        strncpy(h.name, name_ ? name_ : "PANEL", hn::kMaxName);
        h.name[hn::kMaxName] = '\0';
    }
    void onState(const hn::StateRecord& r) override {
        if (mode_ == hn::MODE_MAINTENANCE) return;
        for (Element* e = Element::first(); e; e = e->next())
            if ((e->roles() & kRoleOut) && e->id() == r.id) e->onState(r);
    }
    uint8_t onMode(uint8_t m) override {
        if (m > hn::MODE_WIRING_TEST) m = hn::MODE_SIM;
        mode_ = m;
        for (Element* e = Element::first(); e; e = e->next()) e->onMode(m);
        if (modeHook_) modeHook_(m);
        return m;
    }
    uint8_t onConfig(const uint8_t* p, size_t n) override {
        if (p[0] == hn::CFG_SAVE) return (save_ && save_()) ? 0 : hn::NACK_UNSUPPORTED;
        if (p[0] == hn::CFG_CALIBRATION) {
            if (n < 7) return hn::NACK_BAD_FRAME;
            const uint16_t id = hn::getU16(p + 1);
            bool known = false;
            for (Element* e = Element::first(); e; e = e->next()) {
                if (e->id() != id) continue;
                known = true;
                if (e->calibrate(hn::getU16(p + 3), hn::getU16(p + 5))) return 0;
            }
            return known ? hn::NACK_BAD_VALUE : hn::NACK_UNKNOWN_CONTROL;
        }
        return hn::NACK_UNSUPPORTED;
    }
    void onBusAddressChanged(uint8_t a) override { if (addrHook_) addrHook_(a); }
    uint16_t describeCount() override {
        uint16_t n = 0;
        for (Element* e = Element::first(); e; e = e->next())
            n = static_cast<uint16_t>(n + ((e->roles() & kRoleIn) ? 1 : 0) + ((e->roles() & kRoleOut) ? 1 : 0));
        return n;
    }
    hn::DescribeRecord describeItem(uint16_t i) override {
        hn::DescribeRecord r;
        for (Element* e = Element::first(); e; e = e->next()) {
            for (uint8_t role = kRoleIn; role <= kRoleOut; role = static_cast<uint8_t>(role << 1)) {
                if (!(e->roles() & role)) continue;
                if (i-- == 0) {
                    r.id = e->id();
                    r.role = role == kRoleIn ? hn::DESCRIBE_ROLE_INPUT : hn::DESCRIBE_ROLE_OUTPUT;
                    return r;
                }
            }
        }
        return r;
    }
    uint16_t syncCount() override {
        uint16_t n = 0;
        uint16_t v;
        for (Element* e = Element::first(); e; e = e->next()) if (e->syncValue(v)) n++;
        return n;
    }
    hn::SyncRecord syncItem(uint16_t i) override {
        hn::SyncRecord r;
        uint16_t v;
        for (Element* e = Element::first(); e; e = e->next()) {
            if (!e->syncValue(v)) continue;
            if (i-- == 0) { r.id = e->id(); r.value = v; return r; }
        }
        return r;
    }
    size_t writeSubscribe(uint8_t* buf, size_t cap) override {
        // Prefer the exact list of outputs; fall back to whole panels if it is too long.
        size_t n = 1;
        buf[0] = hn::SUBSCRIBE_CONTROLS;
        for (Element* e = Element::first(); e; e = e->next()) {
            if (!(e->roles() & kRoleOut)) continue;
            if (n + 2 > cap) return subscribePanels(buf, cap);
            hn::putU16(buf + n, e->id());
            n += 2;
        }
        return n;
    }
    void fillDiag(hn::Diag& d) override {
        d.freeRam = detail::freeRam();
        strncpy(d.text, name_ ? name_ : "", sizeof(d.text) - 1);
    }

private:
    uint32_t identityHash() const {
        uint32_t h = 2166136261UL;
        for (uint8_t b : boardId_) { h ^= b; h *= 16777619UL; }
        return h;
    }

    void loadBoardId() {
        if (boardIdSet_) return;
#if defined(ESP32)
        const uint64_t mac = ESP.getEfuseMac();
        for (uint8_t i = 0; i < 8; i++) boardId_[i] = static_cast<uint8_t>(mac >> (8 * i));
#elif defined(__AVR__)
        const uint8_t magic = EEPROM.read(0);
        if (magic == 0xA5) {
            for (uint8_t i = 0; i < 8; i++) boardId_[i] = EEPROM.read(static_cast<int>(i + 1));
        } else {
            uint32_t seed = static_cast<uint32_t>(analogRead(A0)) ^ micros();
            for (uint8_t i = 0; i < 8; i++) {
                seed = seed * 1103515245UL + 12345UL;
                boardId_[i] = static_cast<uint8_t>(seed >> 24);
                EEPROM.update(static_cast<int>(i + 1), boardId_[i]);
            }
            EEPROM.update(0, 0xA5);
        }
#endif
    }

    void start() {
        Element::sink() = &Panel::inputThunk;
        self() = this;
        const uint32_t now = millis();
        for (Element* e = Element::first(); e; e = e->next()) e->begin(now);
        for (DigitalSink* s = DigitalSink::first(); s; s = s->nextSink) s->flush();
    }

    static Panel*& self() { static Panel* p = nullptr; return p; }
    static void inputThunk(uint16_t id, uint8_t action, uint16_t arg) { if (self()) self()->input(id, action, arg); }

    void input(uint16_t id, uint8_t action, uint16_t arg) {
        if (debug_) { printInput(id, action, arg); return; }
        hn::InputRecord r;
        r.id = id;
        r.action = action;
        r.arg = arg;
        node_.queueInput(r);
    }

    size_t subscribePanels(uint8_t* buf, size_t cap) {
        buf[0] = hn::SUBSCRIBE_PANELS;
        size_t n = 1;
        for (Element* e = Element::first(); e; e = e->next()) {
            if (!(e->roles() & kRoleOut)) continue;
            const uint16_t panel = static_cast<uint16_t>(e->id() & 0xFF00);
            bool seen = false;
            for (size_t i = 1; i < n; i += 2) if (hn::getU16(buf + i) == panel) seen = true;
            if (seen) continue;
            if (n + 2 > cap) { buf[0] = hn::SUBSCRIBE_ALL; return 1; }
            hn::putU16(buf + n, panel);
            n += 2;
        }
        return n;
    }

    // ── ASCII debug mode ─────────────────────────────────────────────────
    void printInput(uint16_t id, uint8_t action, uint16_t arg) {
        char name[40];
        controlName(id, name, sizeof(name));
        if (mode_ == hn::MODE_WIRING_TEST) debug_->print(F("WIRING "));
        debug_->print(name);
        ControlInfo row;
        const bool known = findControl(id, row);
        char pos[16];
        switch (action) {
        case hn::ACTION_STEP:
            debug_->print(static_cast<int16_t>(arg) >= 0 ? F(" STEP +") : F(" STEP "));
            debug_->println(static_cast<int16_t>(arg));
            return;
        case hn::ACTION_ANALOG:
            debug_->print('=');
            debug_->println(arg);
            return;
        default:
            if (known) positionName(row, arg, pos, sizeof(pos));
            else { pos[0] = '?'; pos[1] = '\0'; }
            debug_->print('=');
            debug_->println(pos);
        }
    }

    void readDebug() {
        while (debug_->available()) {
            const int c = debug_->read();
            if (c < 0) break;
            if (c == '\r') continue;
            if (c != '\n') {
                if (lineLen_ < sizeof(line_) - 1) line_[lineLen_++] = static_cast<char>(c);
                continue;
            }
            line_[lineLen_] = '\0';
            lineLen_ = 0;
            handleDebugLine();
        }
    }

    void handleDebugLine() {
        if (strcmp(line_, "SYNC") == 0) {
            uint16_t v;
            for (Element* e = Element::first(); e; e = e->next())
                if (e->syncValue(v)) printInput(e->id(), hn::ACTION_SET_POSITION, v);
            return;
        }
        if (strncmp(line_, "MODE ", 5) == 0) {
            const char* m = line_ + 5;
            uint8_t mode = hn::MODE_SIM;
            if (strcmp(m, "LAMP_TEST") == 0) mode = hn::MODE_LAMP_TEST;
            else if (strcmp(m, "MAINTENANCE") == 0) mode = hn::MODE_MAINTENANCE;
            else if (strcmp(m, "WIRING_TEST") == 0) mode = hn::MODE_WIRING_TEST;
            onMode(mode);
            debug_->print(F("MODE "));
            debug_->println(m);
            return;
        }
        char* eq = strchr(line_, '=');
        if (!eq) { debug_->println(F("? use NAME=VALUE, SYNC or MODE <name>")); return; }
        *eq = '\0';
        ControlInfo row;
        if (!findControlByName(line_, row)) { debug_->print(F("? unknown control ")); debug_->println(line_); return; }
        hn::StateRecord r;
        r.id = row.id;
        const char* val = eq + 1;
        if (row.kind == static_cast<uint8_t>(Kind::Text)) {
            r.kind = hn::VALUE_TEXT;
            r.text = reinterpret_cast<const uint8_t*>(val);
            r.textLen = static_cast<uint8_t>(strlen(val));
        } else {
            const int named = positionIndex(row, val);
            r.kind = hn::VALUE_POSITION;
            r.value = named >= 0 ? static_cast<uint16_t>(named) : static_cast<uint16_t>(strtoul(val, nullptr, 10));
            if (row.kind == static_cast<uint8_t>(Kind::Gauge) || row.kind == static_cast<uint8_t>(Kind::Axis)) r.kind = hn::VALUE_ANALOG;
        }
        onState(r);
        debug_->println(F("ok"));
    }

    const char* name_;
    StreamPort port_;
    hn::Node node_;
    uint8_t boardId_[8] = {};
    bool boardIdSet_ = false;
    uint8_t mode_ = hn::MODE_SIM;
    Stream* debug_ = nullptr;
    char line_[48];
    uint8_t lineLen_ = 0;
    SaveHook save_ = nullptr;
    ModeHook modeHook_ = nullptr;
    AddressHook addrHook_ = nullptr;
};

/**
 * @brief RS-485 bus master board (Mega 2560, ESP32, Giga). No panel code needed.
 * @code
 *   Hornet::BusMaster master("LEFT CONSOLE");
 *   void setup() {
 *     master.addSlave(1);                 // fixed bus addresses of your panels
 *     master.addSlave(2);
 *     master.begin(Serial, Serial1, 2);   // USB to PC, RS-485 on Serial1, DE on pin 2
 *   }
 *   void loop() { master.update(); }
 * @endcode
 */
class BusMaster {
public:
    explicit BusMaster(const char* name) : name_(name), core_(usb_, bus_) {}

    bool addSlave(uint8_t address) { return core_.addSlave(address); }
    /// Also find boards that are not in the fixed list (broadcast discovery with random back-off).
    void enableDiscovery(bool on = true) { core_.enableDiscovery(on); }

    template <class U, class B>
    void begin(U& usbSerial, B& busSerial, int8_t dePin, uint32_t usbBaud = 250000, uint32_t busBaud = 250000) {
        usbSerial.begin(usbBaud);
        busSerial.begin(busBaud);
        usb_.attach(&usbSerial, -1);
        bus_.attach(&busSerial, dePin);
        uint8_t id[8] = {};
        const uint32_t h = detail::fnv(name_);
        for (uint8_t i = 0; i < 4; i++) id[i] = static_cast<uint8_t>(h >> (8 * i));
        core_.setIdentity(name_, HORNET_FW_MAJOR, HORNET_FW_MINOR, id);
    }

    void update() { core_.update(millis()); }
    const hn::BusMaster& core() const { return core_; }

private:
    const char* name_;
    StreamPort usb_;
    StreamPort bus_;
    hn::BusMaster core_;
};

} // namespace Hornet
