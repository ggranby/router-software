// RS-485 bus master (Mega 2560, ESP32 or Arduino Giga). No panel code:
// it relays between hornet-link.exe (USB) and the panels on the bus.
//
// List the fixed bus addresses of your panels below. Discovery is optional:
// it also finds boards that are not in the list.

#include <Hornet.h>

#if defined(ESP32)
  #define BUS_SERIAL Serial2      // RX2/TX2
  const int8_t DE_PIN = 4;
#else
  #define BUS_SERIAL Serial1      // Mega: RX1 19 / TX1 18, Giga: RX0/TX0
  const int8_t DE_PIN = 2;
#endif

Hornet::BusMaster master("BUS MASTER");

void setup() {
  master.addSlave(1);   // MASTER ARM
  master.addSlave(2);   // UFC
  master.addSlave(5);   // CAUTION
  // master.enableDiscovery();
  master.begin(Serial, BUS_SERIAL, DE_PIN);
}

void loop() {
  master.update();
}
