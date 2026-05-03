# Hornet Link — Firmware Guide

This guide covers writing and flashing firmware for Open-Hornet panel boards
that connect to the Hornet Link PC bridge.

---

## Supported Boards

| Board | Role | Notes |
|-------|------|-------|
| Arduino Mega 2560 | RS-485 master | 4× hardware UARTs; Serial1 for RS-485 bus |
| Arduino Pro Micro (32U4) | RS-485 slave or standalone | 2× UARTs; compact form factor |
| ESP32 WROOM/WROVER | RS-485 master | 240 MHz dual-core; future WiFi support |
| Arduino Nano | Standalone only | 1× UART; no RS-485 without external buffer |

---

## Library Installation

### Arduino IDE (manual)

1. Copy `libraries/HornetLink/` into your Arduino sketchbook `libraries/` folder.
2. Restart the IDE.
3. Verify: **Sketch → Include Library → HornetLink** should appear.

### arduino-cli

```sh
cp -r libraries/HornetLink "$HOME/Arduino/libraries/"
arduino-cli compile --fqbn arduino:avr:mega sketches/sketch_mega2560_master
```

---

## Writing a Standalone Panel Sketch

A standalone panel connects directly to a PC USB port (no RS-485).

```cpp
#include <HornetLink.h>

// Which DCS-BIOS fields this panel needs
static const hl_subscription_t kSubs[] = {
    { 0x740A, 0xFFFF, 0 },   // UFC_COMM1 + COMM2 display
    { 0x7406, 0xFFFF, 0 },   // UFC scratchpad number
};

// Called when a subscribed address changes
void onUpdate(uint16_t addr, uint16_t value) {
    // Update your display / LEDs here
}

HornetLinkSlave hl(Serial1, 4); // dummy — standalone uses Serial directly

void setup() {
    Serial.begin(500000);
    // For standalone, use slave with busAddr=0 and pc stream = &Serial
    // See HornetLinkMaster for simplified standalone usage
}

void loop() {
    // Receive bytes from Serial, feed to ExportParser
    // (use the provided HornetLink helpers)
}
```

For the full standalone pattern, start from `sketch_pro_micro_slave.ino` and
remove all RS-485 bus code (use `Serial` directly instead of `Serial1`).

---

## Writing an RS-485 Slave Sketch

```cpp
#include <HornetLink.h>

static constexpr uint8_t kBusAddr = 1; // Unique address per slave
static constexpr uint8_t kDirPin  = 4;

static const hl_subscription_t kSubs[] = {
    { 0x7406, 0xFFFF, 0 },
};

HornetLinkSlave hl(Serial1, kDirPin);

void onUpdate(uint16_t addr, uint16_t val) {
    // addr == 0x7406: UFC scratchpad number
}

void setup() {
    Serial1.begin(250000, SERIAL_8N1);
    hl.begin(kBusAddr, "MyPanel", kSubs, ARRAYSIZE(kSubs),
             HL_FLAG_RS485_SLAVE | HL_FLAG_BIDIR, onUpdate);
}

void loop() {
    hl.update();
    // Send import:
    // hl.sendImport("SET MASTER_ARM_SW 1");
}
```

---

## Writing an RS-485 Master Sketch

See `sketch_mega2560_master.ino` or `sketch_esp32_master.ino`.  The master:

1. Connects to the PC via USB-CDC (Serial at 500 000 baud).
2. Opens the RS-485 bus on a second UART (Serial1 at 250 000 baud).
3. Reports `HL_FLAG_RS485_MASTER` in the pong frame.
4. Polls slave addresses and forwards filtered DCS-BIOS frames to each slave.
5. Relays import lines from slaves back to the PC.

---

## Capability Flags

| Flag macro | Value | Use case |
|------------|-------|----------|
| `HL_FLAG_RS485_MASTER` | 0x01 | Board drives an RS-485 slave bus |
| `HL_FLAG_RS485_SLAVE`  | 0x02 | Board is managed by a master |
| `HL_FLAG_BIDIR`        | 0x04 | Board can send import commands to DCS |

Combine with bitwise OR:
```cpp
HL_FLAG_RS485_MASTER | HL_FLAG_BIDIR
```

---

## Subscription Tips

- Use `{ 0xFFFF, 0xFFFF, 0 }` as the only subscription to receive every changed
  address (legacy / wildcard mode).  Useful for prototyping but uses more UART
  bandwidth.
- Subscribe only to the addresses your panel actually displays.  This reduces
  serial traffic and processing time on the microcontroller.
- The bridge caches per-device subscription lists in `device_profiles.json` so
  they persist across sessions.

---

## Operating Modes

When the operator changes the mode via the Hornet Link UI the bridge sends
`AA DE AD 04 [mode]` to all COM ports.  The HornetLink library handles parsing
and acknowledgement automatically.  Override `onModeChange()` to react:

```cpp
class MyPanel : public HornetLinkSlave {
protected:
    void onModeChange(HornetLinkMode mode) override {
        if (mode == HornetLinkMode::Preflight) {
            // Illuminate all lights for BIT
        }
    }
};
```

---

## Sending Import Commands

Import commands let a panel change a DCS cockpit control:

```cpp
// Inside a button-press ISR or debounce handler:
hl.sendImport("SET UFC_KEY_1 1");
```

The bridge only forwards imports when in **Sim** mode.  In Preflight or
Maintenance mode the commands are silently dropped.

Import commands use the DCS-BIOS command syntax:
```
SET <CONTROL_IDENTIFIER> <VALUE>
```

Valid identifiers and value ranges are listed in `lua/modules/FA-18C.lua` and
the DCS-BIOS control reference JSON.

---

## RS-485 Wiring Reference

```
MAX485 / MAX3485 connections (Mega 2560 example):

  Mega TX1 (pin 18) ──→ MAX485 DI
  Mega RX1 (pin 19) ←── MAX485 RO
  Mega GND          ──→ MAX485 GND
  Mega 5V           ──→ MAX485 VCC
  Mega pin 2        ──→ MAX485 DE + /RE (tied together)

  MAX485 A ──→ bus A (twisted pair +)
  MAX485 B ──→ bus B (twisted pair −)

  120 Ω termination resistor between A and B at both ends of the bus.
```

For the Pro Micro slave (3.3 V logic with MAX3485):
```
  Pro Micro TX1 ──→ MAX3485 DI (3.3 V compatible)
  Pro Micro RX1 ←── MAX3485 RO
  Pro Micro pin 4 → MAX3485 DE + /RE
```

---

## Timing and Baud Rate

| Parameter | Value |
|-----------|-------|
| PC ↔ master USB baud | 500 000 baud |
| RS-485 sub-bus baud | 250 000 baud |
| Serial format | 8N1 |
| DTR / RTS | Disabled (bridge sets these LOW) |
| Write timeout | 50 ms (configurable in `main.cpp`) |

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| No handshake | Wrong baud rate | Ensure `Serial.begin(500000)` on master |
| Slave not responding | Bus address conflict | Set unique `kBusAddr` per slave |
| Garbled data | Missing termination | Add 120 Ω resistors at both ends of RS-485 bus |
| Import not received by DCS | Bridge not in Sim mode | Check mode toolbar buttons in UI |
| Panel shows stale data | Subscriptions not set | Verify subscription addresses match DCS-BIOS reference |
