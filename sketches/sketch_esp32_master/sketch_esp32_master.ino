/**
 * sketch_esp32_master.ino
 * Hornet Link RS-485 Master — ESP32 (WROOM / WROVER)
 * ===========================================================
 * Runs as a drop-in alternative to the Mega 2560 master.
 * Uses UART2 for the RS-485 bus (GPIO16 RX, GPIO17 TX).
 * Connects to the PC via USB CDC or WiFi (future).
 *
 * WIRING
 * ─────────────────────────────────────────────────────────
 *  GPIO16 (RX2) → MAX3485 RO
 *  GPIO17 (TX2) → MAX3485 DI
 *  GPIO4        → MAX3485 /RE + DE (direction control)
 *  USB-OTG / USB-CDC → PC (hornet-link.exe COM port)
 *
 * FEATURES vs MEGA 2560
 * ─────────────────────────────────────────────────────────
 *  + 240 MHz dual-core — handles multiple slaves faster
 *  + Large PSRAM available for buffering
 *  + FreeRTOS: RS-485 bus polling runs on Core 0,
 *    USB serial runs on Core 1 (Arduino default)
 *  + Future: Wi-Fi / BLE gateway mode for wireless panels
 *
 * LICENSE: Apache-2.0
 */

#include "HornetLink.h"
#include "driver/uart.h"

// ── Configuration ────────────────────────────────────────
static constexpr uint8_t  kRS485DirPin  = 4;
static constexpr int      kRxPin        = 16;
static constexpr int      kTxPin        = 17;
static constexpr uint32_t kPcBaud       = 500000UL;
static constexpr uint32_t kBusBaud      = 250000UL;

static const char kDeviceName[] = "ESP32Master";

// ── Subscriptions ────────────────────────────────────────
static const hl_subscription_t kSubscriptions[] = {
    { 0xFFFF, 0xFFFF, 0 }  // wildcard — receive everything
};

// ── HornetLink master instance ───────────────────────────
// Serial2 is UART2 on ESP32 (GPIO16/17 by default)
HornetLinkMaster hl(Serial2, kRS485DirPin);

// ── Arduino lifecycle ────────────────────────────────────
void setup() {
    // PC link
    Serial.begin(kPcBaud);

    // RS-485 bus
    Serial2.begin(kBusBaud, SERIAL_8N1, kRxPin, kTxPin);
    pinMode(kRS485DirPin, OUTPUT);
    digitalWrite(kRS485DirPin, LOW);

    hl.begin(&Serial, kDeviceName,
             kSubscriptions, ARRAYSIZE(kSubscriptions),
             HL_FLAG_RS485_MASTER | HL_FLAG_BIDIR);
}

void loop() {
    hl.update();
}
