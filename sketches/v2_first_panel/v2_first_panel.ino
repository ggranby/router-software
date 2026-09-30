// Your first Hornet Link panel: one switch and one light.
// Step-by-step guide: docs/FIRST_PANEL.md
//
// Wiring (Mega / Giga):
//   - Toggle switch between pin 4 and GND
//   - LED + 330 ohm resistor between pin 9 and GND
//
// ESP32:
//   - Toggle switch between pin 4 and GND
//   - LED + 330 ohm resistor between pin 25 and GND
//
// Upload, open the Serial Monitor at 115200 baud (line ending: Newline) and
// flip the switch: you will see  MASTER_ARM.MASTER_ARM=ARM
// Type  MASTER_ARM.READY_LT=1  and press Enter to light the LED.
// When it works, change beginDebug() to beginUsb() and start hornet-link.exe.

#include <Hornet.h>

using namespace Hornet;

Switch masterArm(MasterArm::MasterArm, 4);    // pin 4 closed = ARM
Lamp   readyLight(MasterArm::ReadyLt, output(
#if defined(ESP32)
    25
#else
    9
#endif
));

Panel panel("FIRST PANEL");

void setup() {
  panel.beginDebug(Serial);     // plain text in the Serial Monitor
  // panel.beginUsb(Serial);    // talk to hornet-link.exe instead
}

void loop() {
  panel.update();
}
