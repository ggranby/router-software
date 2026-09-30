// OpenHornet-style Master Arm panel template (Pro Micro, Mega, ESP32, Giga).
// Fill in your pins; nothing else needs to change. The ESP32 map avoids the
// onboard SPI flash pins (6-11).
//
// Choose how the board talks to the PC by uncommenting ONE line in setup():
//   USB       - board plugged into the PC
//   RS-485    - board is a slave on the bus (fixed address below)
//   DEBUG     - plain text in the Serial Monitor (no PC software needed)

#include <Hornet.h>

using namespace Hornet;

// ── Bus settings (only used with RS-485) ──────────────────────────────────
const uint8_t BUS_ADDRESS = 1;   // OpenHornet fixed address for this panel
#if defined(ESP32)
const int8_t  RS485_DE_PIN = 27; // DE + /RE of the MAX485
#else
const int8_t  RS485_DE_PIN = 2;  // DE + /RE of the MAX485
#endif

// ── Inputs ────────────────────────────────────────────────────────────────
#if defined(ESP32)
Switch masterArm(MasterArm::MasterArm, 16);         // closed = ARM
Button aaButton(MasterArm::ModeAa, 17);
Button agButton(MasterArm::ModeAg, 18);
Button fireExt(MasterArm::FireExt, 19);
Button emerJett(MasterArm::EmerJett, 21);
#else
Switch masterArm(MasterArm::MasterArm, 4);          // closed = ARM
Button aaButton(MasterArm::ModeAa, 5);
Button agButton(MasterArm::ModeAg, 6);
Button fireExt(MasterArm::FireExt, 7);
Button emerJett(MasterArm::EmerJett, 8);
#endif

// ── Lights ────────────────────────────────────────────────────────────────
Lamp aaLight(MasterArm::AaLt, output(
#if defined(ESP32)
    22
#else
    9
#endif
));
Lamp agLight(MasterArm::AgLt, output(
#if defined(ESP32)
    23
#else
    10
#endif
));
Lamp readyLight(MasterArm::ReadyLt, output(
#if defined(ESP32)
    25
#else
    14
#endif
));
Lamp dischLight(MasterArm::DischLt, output(
#if defined(ESP32)
    26
#else
    15
#endif
));

Panel panel("MASTER ARM");

void setup() {
  panel.beginUsb(Serial);
  // panel.beginRs485(Serial1, BUS_ADDRESS, RS485_DE_PIN);
  // panel.beginDebug(Serial);
}

void loop() {
  panel.update();
}
