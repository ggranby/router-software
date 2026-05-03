/**
 * sketch_mega2560_master.ino
 * Hornet Link RS-485 Master — Arduino Mega 2560
 * ==========================================================
 * This sketch runs on an Arduino Mega 2560 wired to the PC
 * via USB (USB-CDC serial at 500 000 baud) and to a chain of
 * slave panels via RS-485 (Serial1 at 250 000 baud, 8N1).
 *
 * WIRING (example)
 * ─────────────────────────────────────────────────────────
 *  USB   → PC Serial port (hornet-link.exe sees this COM port)
 *  Serial1 TX  → MAX485 DE + DI
 *  Serial1 RX  → MAX485 RO
 *  Pin 2       → MAX485 /RE + DE (direction control)
 *
 * HANDSHAKE
 * ─────────────────────────────────────────────────────────
 * On power-up the master sends AA DE AD 01 to the PC, waits
 * for the capability pong (AA DE AD 02 ...) and replies with
 * the ack (AA DE AD 03).  It declares itself as an RS-485
 * master by setting flags = 0x01.  If any slaves are already
 * enumerated it includes the slave list in the pong frame so
 * the PC can build per-slave subscription filters.
 *
 * IMPORT (device → DCS)
 * ─────────────────────────────────────────────────────────
 * The master forwards import lines received from slaves up to
 * the PC as ASCII lines terminated with '\\n':
 *    SET UFC_KEY_1 1\\n
 *    SET MASTER_ARM_SW 1\\n
 *
 * LICENSE: Apache-2.0
 */

#include "HornetLink.h"

// ── Pin / port configuration ─────────────────────────────
static constexpr uint8_t  kRS485DirPin  = 2;
static constexpr uint32_t kPcBaud       = 500000UL;
static constexpr uint32_t kBusBaud      = 250000UL;

// ── Static device name ───────────────────────────────────
static const char kDeviceName[] = "Mega2560Master";

// ── HornetLink master instance ───────────────────────────
HornetLinkMaster hl(Serial1, kRS485DirPin);

// ── Subscriptions ────────────────────────────────────────
// Request the full stream: wildcard subscription 0xFFFF/0xFFFF.
static const hl_subscription_t kSubscriptions[] = {
    { 0xFFFF, 0xFFFF, 0 }
};

// ── Arduino lifecycle ────────────────────────────────────
void setup() {
    Serial.begin(kPcBaud);
    Serial1.begin(kBusBaud, SERIAL_8N1);
    pinMode(kRS485DirPin, OUTPUT);
    digitalWrite(kRS485DirPin, LOW); // RX mode

    hl.begin(&Serial, kDeviceName,
             kSubscriptions, ARRAYSIZE(kSubscriptions),
             HL_FLAG_RS485_MASTER | HL_FLAG_BIDIR);
}

void loop() {
    hl.update();
}
