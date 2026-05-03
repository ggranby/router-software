/**
 * sketch_pro_micro_slave.ino
 * Hornet Link RS-485 Slave — SparkFun Pro Micro (ATmega32U4)
 * ===========================================================
 * Runs on a panel slave board connected via RS-485 to a
 * Mega 2560 (or ESP32) master.  Does NOT connect to the PC
 * directly; all data passes through the master.
 *
 * WIRING
 * ─────────────────────────────────────────────────────────
 *  Serial1 TX → MAX485 DE + DI
 *  Serial1 RX → MAX485 RO
 *  Pin 4      → MAX485 /RE + DE (direction control)
 *
 * RS-485 BUS ADDRESS
 * ─────────────────────────────────────────────────────────
 * Set kBusAddress to a unique value (1–254) for each slave
 * on the bus.  Address 0 is reserved (broadcast); 255 is
 * reserved (unassigned / factory state).
 *
 * The slave address should be configured via hardware DIP
 * switches in production.  Read them in setup() and pass to
 * hl.begin().
 *
 * SUBSCRIPTIONS
 * ─────────────────────────────────────────────────────────
 * Replace the example UFC entries with the fields your panel
 * actually needs.  The master propagates these to the PC
 * during handshake so the bridge can filter per-slave.
 *
 * LICENSE: Apache-2.0
 */

#include "HornetLink.h"

// ── Configuration ────────────────────────────────────────
static constexpr uint8_t  kBusAddress   = 1;       // Change per panel
static constexpr uint8_t  kRS485DirPin  = 4;
static constexpr uint32_t kBusBaud      = 250000UL;

static const char kDeviceName[] = "ProMicroSlave";

// ── Subscriptions (example: UFC scratchpad + COMM display)
static const hl_subscription_t kSubscriptions[] = {
    { 0x7406, 0xFFFF, 0  },  // UFC_SCRATCHPAD_NUMBER
    { 0x7408, 0xFFFF, 0  },  // UFC_SCRATCHPAD_STRING_1 + _2
    { 0x740A, 0xFFFF, 0  },  // UFC_COMM1 + COMM2 display
};

// ── HornetLink slave instance ────────────────────────────
HornetLinkSlave hl(Serial1, kRS485DirPin);

// ── Panel state ──────────────────────────────────────────
uint16_t ufc_scratch_num  = 0;
uint8_t  ufc_scratch_str1 = 0;
uint8_t  ufc_scratch_str2 = 0;
uint8_t  ufc_comm1        = 0;
uint8_t  ufc_comm2        = 0;

// ── Called by HornetLink when a subscribed address changes
void onStateUpdate(uint16_t address, uint16_t value) {
    switch (address) {
    case 0x7406: ufc_scratch_num  = value; break;
    case 0x7408:
        ufc_scratch_str1 = value & 0xFF;
        ufc_scratch_str2 = (value >> 8) & 0xFF;
        break;
    case 0x740A:
        ufc_comm1 = value & 0xFF;
        ufc_comm2 = (value >> 8) & 0xFF;
        break;
    }
    renderDisplay();
}

void renderDisplay() {
    // TODO: drive 7-segment drivers, TM1637, or I²C display here.
}

// ── Arduino lifecycle ────────────────────────────────────
void setup() {
    Serial1.begin(kBusBaud, SERIAL_8N1);
    pinMode(kRS485DirPin, OUTPUT);
    digitalWrite(kRS485DirPin, LOW);

    hl.begin(kBusAddress, kDeviceName,
             kSubscriptions, ARRAYSIZE(kSubscriptions),
             HL_FLAG_RS485_SLAVE | HL_FLAG_BIDIR,
             onStateUpdate);
}

void loop() {
    hl.update();

    // Example import: send when a button is pressed.
    // hl.sendImport("SET UFC_KEY_1 1");
}
