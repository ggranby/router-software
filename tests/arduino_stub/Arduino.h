#pragma once

#include <stdint.h>

constexpr uint8_t INPUT = 0;
constexpr uint8_t OUTPUT = 1;
constexpr uint8_t INPUT_PULLUP = 2;
constexpr uint8_t LOW = 0;
constexpr uint8_t HIGH = 1;
constexpr uint8_t MSBFIRST = 1;

inline void pinMode(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t) { return HIGH; }
inline void digitalWrite(uint8_t, uint8_t) {}
inline int analogRead(uint8_t) { return 0; }
inline void analogWrite(uint8_t, uint8_t) {}
inline void delayMicroseconds(unsigned int) {}
inline void shiftOut(uint8_t, uint8_t, uint8_t, uint8_t) {}
