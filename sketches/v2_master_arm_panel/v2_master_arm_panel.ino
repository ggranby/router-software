// OpenHornet-style Master Arm panel template (Pro Micro, Mega, ESP32, Giga).
// Fill in your pins; nothing else needs to change.
//
// Choose how the board talks to the PC by uncommenting ONE line in setup():
//   USB       - board plugged into the PC
//   RS-485    - board is a slave on the bus (fixed address below)
//   DEBUG     - plain text in the Serial Monitor (no PC software needed)

#include <Hornet.h>

using namespace Hornet;

// ── Bus settings (only used with RS-485) ──────────────────────────────────
const uint8_t BUS_ADDRESS = 1;   // OpenHornet fixed address for this panel
const int8_t  RS485_DE_PIN = 2;  // DE + /RE of the MAX485

// ── Inputs ────────────────────────────────────────────────────────────────
Switch masterArm(MasterArm::MasterArm, 4);          // closed = ARM
Button aaButton(MasterArm::ModeAa, 5);
Button agButton(MasterArm::ModeAg, 6);
Button fireExt(MasterArm::FireExt, 7);
Button emerJett(MasterArm::EmerJett, 8);

// ── Lights ────────────────────────────────────────────────────────────────
Lamp aaLight(MasterArm::AaLt, output(9));
Lamp agLight(MasterArm::AgLt, output(10));
Lamp readyLight(MasterArm::ReadyLt, output(14));
Lamp dischLight(MasterArm::DischLt, output(15));

Panel panel("MASTER ARM");

void setup() {
  panel.beginUsb(Serial);
  // panel.beginRs485(Serial1, BUS_ADDRESS, RS485_DE_PIN);
  // panel.beginDebug(Serial);
}

void loop() {
  panel.update();
}
