// Up Front Controller template: keypad matrix, option buttons, volume pots,
// channel encoders and text displays. Written for a Mega 2560 / ESP32 / Giga;
// trim it (fewer displays) for a Pro Micro.
//
// Text displays: this sketch prints them to Serial1 so you can see them
// arriving. Replace showScratchpad()/showOption() with your display driver
// (MAX7219, HT16K33, OLED...).

#include <Hornet.h>

using namespace Hornet;

// ── Keypad: 4 rows x 3 columns (1 2 3 / 4 5 6 / 7 8 9 / CLR 0 ENT) ────────
#if defined(ESP32)
// GPIO34-39 are input-only; the contacts assigned there require external
// pull-up resistors.
const uint8_t ROWS[] = {18, 19, 21, 22};
const uint8_t COLS[] = {23, 25, 26};
#else
const uint8_t ROWS[] = {22, 23, 24, 25};
const uint8_t COLS[] = {26, 27, 28};
#endif
Matrix keypad(ROWS, COLS);

Button key1(UFC::Key1, keypad.key(0, 0));
Button key2(UFC::Key2, keypad.key(0, 1));
Button key3(UFC::Key3, keypad.key(0, 2));
Button key4(UFC::Key4, keypad.key(1, 0));
Button key5(UFC::Key5, keypad.key(1, 1));
Button key6(UFC::Key6, keypad.key(1, 2));
Button key7(UFC::Key7, keypad.key(2, 0));
Button key8(UFC::Key8, keypad.key(2, 1));
Button key9(UFC::Key9, keypad.key(2, 2));
Button keyClr(UFC::KeyClr, keypad.key(3, 0));
Button key0(UFC::Key0, keypad.key(3, 1));
Button keyEnt(UFC::KeyEnt, keypad.key(3, 2));

// ── Option select buttons and function keys on direct pins ────────────────
#if defined(ESP32)
Button os1(UFC::Os1, 16);
Button os2(UFC::Os2, 2);
Button os3(UFC::Os3, 4);
Button os4(UFC::Os4, 5);
Button os5(UFC::Os5, 12);
Button ap(UFC::Ap, 13);
Button iff(UFC::Iff, 14);
Button tcn(UFC::Tcn, 15);
Button ils(UFC::Ils, 32);
Button dl(UFC::Dl, 17);
Button bcn(UFC::Bcn, 27);
Button onOff(UFC::Onoff, 35);
Switch adf(UFC::Adf, 33, none, 34);
#else
Button os1(UFC::Os1, 30);
Button os2(UFC::Os2, 31);
Button os3(UFC::Os3, 32);
Button os4(UFC::Os4, 33);
Button os5(UFC::Os5, 34);
Button ap(UFC::Ap, 35);
Button iff(UFC::Iff, 36);
Button tcn(UFC::Tcn, 37);
Button ils(UFC::Ils, 38);
Button dl(UFC::Dl, 39);
Button bcn(UFC::Bcn, 40);
Button onOff(UFC::Onoff, 41);
Switch adf(UFC::Adf, 42, none, 43);                 // ADF1 / OFF / ADF2
#endif

// ── Knobs ─────────────────────────────────────────────────────────────────
Pot comm1Vol(UFC::Comm1Vol, A0);
Pot comm2Vol(UFC::Comm2Vol, A1);
Pot brightness(UFC::Brt, A2);
Encoder comm1Channel(UFC::Comm1Channel, 16, 17);
Encoder comm2Channel(UFC::Comm2Channel,
#if defined(ESP32)
                     36, 39);
#else
                     46, 47);
#endif
#if defined(ESP32)
Button comm1Pull(UFC::Comm1Pull, 35);
Button comm2Pull(UFC::Comm2Pull, 34);
#else
Button comm1Pull(UFC::Comm1Pull, 44);
Button comm2Pull(UFC::Comm2Pull, 45);
#endif

// ── Displays ──────────────────────────────────────────────────────────────
void showScratchpad(const char* text, uint8_t) { Serial1.print(F("SCRATCHPAD ")); Serial1.println(text); }
void showOption(const char* text, uint8_t)     { Serial1.print(F("OPTION ")); Serial1.println(text); }
void showComm(const char* text, uint8_t)       { Serial1.print(F("COMM ")); Serial1.println(text); }

TextDisplay scratchpad(UFC::ScratchpadNumber, showScratchpad);
TextDisplay option1(UFC::OptionDisplay1, showOption);
TextDisplay option2(UFC::OptionDisplay2, showOption);
TextDisplay option3(UFC::OptionDisplay3, showOption);
TextDisplay option4(UFC::OptionDisplay4, showOption);
TextDisplay option5(UFC::OptionDisplay5, showOption);
TextDisplay comm1(UFC::Comm1Display, showComm);
TextDisplay comm2(UFC::Comm2Display, showComm);

Panel panel("UFC");

void setup() {
  Serial1.begin(115200);
  panel.beginUsb(Serial);
  // panel.beginRs485(Serial2, 2, 4);   // bus address 2, DE pin 4
}

void loop() {
  panel.update();
}
