// Left advisory / caution lights on 74HC595 shift registers, plus the
// glareshield master caution and fire lights. Pro Micro friendly.
//
// Two 74HC595 chips: latch pin 10, clock pin 15, data pin 16.
// Lamp test from the bridge lights every lamp.

#include <Hornet.h>

using namespace Hornet;

Hc595 lamps(10, 15, 16, 2);

Lamp lBleed(LhAdvisory::LBleed, lamps.bit(0));
Lamp rBleed(LhAdvisory::RBleed, lamps.bit(1));
Lamp spdBrk(LhAdvisory::SpdBrk, lamps.bit(2));
Lamp stby(LhAdvisory::Stby, lamps.bit(3));
Lamp lBarRed(LhAdvisory::LBarRed, lamps.bit(4));
Lamp rec(LhAdvisory::Rec, lamps.bit(5));
Lamp lBarGreen(LhAdvisory::LBarGreen, lamps.bit(6));
Lamp xmit(LhAdvisory::Xmit, lamps.bit(7));
Lamp aspjOh(LhAdvisory::AspjOh, lamps.bit(8));
Lamp go(LhAdvisory::Go, lamps.bit(9));
Lamp noGo(LhAdvisory::NoGo, lamps.bit(10));

Lamp masterCaution(FireCaution::MasterCautionLt, lamps.bit(11));
Lamp fireLeft(FireCaution::FireLeftLt, lamps.bit(12));
Lamp fireRight(FireCaution::FireRightLt, lamps.bit(13));
Lamp fireApu(FireCaution::FireApuLt, lamps.bit(14));

Button masterCautionReset(FireCaution::MasterCautionReset, 4);

Panel panel("CAUTION");

void setup() {
  panel.beginRs485(Serial1, 5, 2);   // bus address 5, DE pin 2
  // panel.beginUsb(Serial);
}

void loop() {
  panel.update();
}
