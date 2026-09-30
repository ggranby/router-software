# Your first Hornet Link panel

This guide takes you from a bare Arduino to a switch and a light that work
with DCS. You don't need to know anything about DCS-BIOS, addresses or
protocols. Every control has a name, and the library does the rest.

## What you need

- Any supported board: Pro Micro / Leonardo, Mega 2560, ESP32 or Arduino Giga
- One toggle switch, one LED, one 330 Ω resistor, some wire
- The Arduino IDE (or arduino-cli)
- This repository's `libraries/HornetLink` folder copied into your Arduino
  `libraries` folder

## Step 1: wire it

```
pin 4 ──── switch ──── GND        (no resistor needed; the library turns on the pull-up)
pin 9 ──── 330 Ω ──── LED (+) ──── LED (−) ──── GND
```

## Step 2: open the example

Open `sketches/v2_first_panel/v2_first_panel.ino`:

```cpp
#include <Hornet.h>
using namespace Hornet;

Switch masterArm(MasterArm::MasterArm, 4);          // pin 4 closed = ARM
Lamp   readyLight(MasterArm::ReadyLt, output(9));

Panel panel("FIRST PANEL");

void setup() { panel.beginDebug(Serial); }
void loop()  { panel.update(); }
```

Each line says **what** the part is (`Switch`, `Lamp`), **which cockpit
control** it is (`MasterArm::MasterArm`), and **where** it's wired (`4`,
`output(9)`). Every control name is listed in
[F18C_CONTROL_REFERENCE.md](F18C_CONTROL_REFERENCE.md). Your editor's
autocomplete also works: type `UFC::` and pick from the list.

## Step 3: upload and test without DCS

1. Upload the sketch.
2. Open the Serial Monitor at **115200** baud, line ending **Newline**.
3. Flip the switch. You should see `MASTER_ARM.MASTER_ARM=ARM` and
   `MASTER_ARM.MASTER_ARM=SAFE`.
4. Type `MASTER_ARM.READY_LT=1` and press Enter. The LED lights. `=0` turns it
   off.
5. Type `MODE LAMP_TEST`. The LED lights. `MODE SIM` returns to normal.

If the name is wrong or nothing prints, check the troubleshooting table below.

## Step 4: connect to the PC software

1. Change `panel.beginDebug(Serial);` to `panel.beginUsb(Serial);` and upload.
2. Start `hornet-link.exe` and select the board's COM port.
3. Install the exporter: copy `Programs/dcsbios-serial-bridge/lua/HornetLinkNative.lua`
   to `Saved Games\DCS\Scripts\` and add the two lines from the top of that
   file to the end of `Saved Games\DCS\Scripts\Export.lua`. You can also paste
   the whole file at the end of `Export.lua`.

> **Status:** the firmware, the exporter and the bridge's protocol logic are
> complete and tested. The bridge window doesn't use the v2 path yet (stage 2
> in [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)). Until then, test panels in
> debug mode.

## Step 5: build a real panel

Copy a template and change only the pin numbers:

| Template | Panel | Boards |
|---|---|---|
| `sketches/v2_master_arm_panel` | Master Arm | Pro Micro, Mega, ESP32, Giga |
| `sketches/v2_ufc_panel` | UFC keypad (switch matrix), display, knobs | Mega, ESP32, Giga |
| `sketches/v2_caution_panel` | Caution lights on 74HC595 shift registers | Pro Micro, Mega |
| `sketches/v2_bus_master` | RS-485 bus master (no panel code) | Mega, ESP32, Giga |

### Element types

| Element | Example | Notes |
|---|---|---|
| `Switch` | `Switch apu(Apu::ApuControl, 22);` | 2-position: one contact. 3-position: one contact per position in reference order, `none` where there is no contact, e.g. `Switch crank(Apu::EngineCrank, 3, none, 5);` |
| `Switch` + `holdCoil` | `apu.holdCoil(output(30), Apu::ApuControl.ON);` | Magnetically held switches: the coil holds while DCS reports that position |
| `Selector` | `Selector sym(HudControl::SymRej, 30, 31, 32);` | Rotary selector; or one analog pin with a resistor ladder: `Selector sym(HudControl::SymRej, analogPin(A1));` |
| `Button` | `Button key1(UFC::Key1, keypad.key(0, 1));` | Sends PRESS and RELEASE |
| `Pot` | `Pot vol(UFC::Comm1Vol, analogPin(A0));` | Smoothing, deadband and rate limit built in. `.reverse()` if it turns the wrong way |
| `Encoder` | `Encoder ch(UFC::Comm1Channel, 2, 3);` | Detent counting built in. `.detents(2)` for half-step encoders, `.accelerate()` for fast spinning |
| `Lamp` | `Lamp l(MasterArm::AaLt, output(9));` | `output(9).inverted()` for active-low LEDs |
| `Gauge` | `Gauge heading(StbyCompass::Heading, 6);` | PWM pin, or a callback with an optional calibration curve |
| `TextDisplay` | `TextDisplay sp(UFC::ScratchpadNumber, drawFn);` | Your function receives fixed-length text |

The library checks bindings when it compiles. A `Pot` bound to a switch, or
a 3-position switch with two contacts, fails with a readable message such as
`Hornet::Switch: give one contact per position ...`.

### Moving a switch to a different chip

The wiring is only the second argument. Swapping it is a one-line change:

```cpp
Switch masterArm(MasterArm::MasterArm, 4);                  // direct pin
Switch masterArm(MasterArm::MasterArm, shiftIn.bit(12));    // 74HC165 chain
Switch masterArm(MasterArm::MasterArm, expander.pin(3));    // MCP23017 (HornetMcp23017.h)
Switch masterArm(MasterArm::MasterArm, keypad.key(2, 1));   // switch matrix
Switch masterArm(MasterArm::MasterArm, mux.contact(7));     // CD4067 multiplexer
```

## Step 6: put it on the RS-485 bus

1. In the panel sketch, use `panel.beginRs485(Serial1, BUS_ADDRESS, DE_PIN);`
   with the panel's fixed OpenHornet address.
2. In `sketches/v2_bus_master`, add that address with `master.addSlave(...)`.
3. Wire A-A, B-B, GND-GND, with 120 Ω termination at both ends of the bus.

The master polls each slave in turn, so slaves never talk over each other.
See [PROTOCOL_V2.md](PROTOCOL_V2.md) for timing and the slot budget.

## Troubleshooting

| Symptom / message | Cause | Fix |
|---|---|---|
| Nothing prints in the Serial Monitor | Wrong baud rate, or `beginUsb` instead of `beginDebug` | 115200 baud, `beginDebug(Serial)` |
| Switch prints the opposite position | Contact wired to the other throw | Move the wire, or give the contacts in the other order |
| Pot value jumps around | Noisy wiring | Shorter wires, a 100 nF capacitor from wiper to GND, or `vol.filter.smoothing = 5;` in `setup()` |
| Encoder moves 2 or 4 steps per click | Encoder type | `.detents(2)` or `.detents(1)` |
| NACK `UNKNOWN_CONTROL` | Library and bridge use different catalogues | Update both from the same release |
| NACK `BAD_VALUE` | Value doesn't fit the control (e.g. position 3 on a 2-position switch) | Check the element type in [F18C_CONTROL_REFERENCE.md](F18C_CONTROL_REFERENCE.md) |
| NACK `WRONG_MODE` | Lamp test or maintenance mode is on | Switch back to SIM |
| Warning "built with a different control catalogue" | Sketch compiled with an older/newer library | Re-upload with the current library |
| DIAG `crc_errors` rising | Electrical noise on RS-485 | Termination, twisted pair, common GND |
| DIAG `overflows` rising | Baud rate mismatch or two boards driving the bus | Same baud everywhere; check for duplicate addresses |
| DIAG `dropped_inputs` > 0 | No ACK from the bridge (bridge not running, cable unplugged) | Start the bridge; check the cable |
| `BUS_EVENT DROP` for a slave | Slave not answering polls | Power, address, DE pin wiring |
| `BUS_EVENT CONFLICT` | Two boards with the same bus address | Give each board its own address |
| Sync list shows switches that disagree | Cockpit doesn't match DCS after spawn | Move each listed switch to the position shown. DCS is the source of truth |
