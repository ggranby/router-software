/**
 * @file HornetTypes.h
 * @brief Core types shared by the generated catalogue, the Arduino library and
 *        the PC bridge. Plain C++11, no Arduino or STL dependency.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ── Flash storage portability ─────────────────────────────────────────────
// AVR (Pro Micro, Mega 2560) keeps constant tables in flash and needs special
// reads. Every other target (ESP32, Giga, host PC) reads flash like RAM.
#if defined(__AVR__)
  #include <avr/pgmspace.h>
  #define HN_PROGMEM PROGMEM
  #define hn_memcpy_P(dst, src, n) memcpy_P((dst), (src), (n))
  #define hn_read_char_P(p) static_cast<char>(pgm_read_byte(p))
#else
  #define HN_PROGMEM
  #define hn_memcpy_P(dst, src, n) memcpy((dst), (src), (n))
  #define hn_read_char_P(p) (*(p))
#endif

namespace Hornet {

/// What kind of cockpit element a catalogue control is.
enum class Kind : uint8_t {
    Switch   = 0, ///< 2- or 3-position toggle switch (incl. guards and magnetic switches)
    Selector = 1, ///< Rotary selector with N named positions
    Button   = 2, ///< Momentary pushbutton (RELEASED / PRESSED)
    Axis     = 3, ///< Absolute analog input 0-65535 (potentiometer; encoder in absolute mode)
    Rotary   = 4, ///< Relative-only input (encoder steps, no readable position)
    Lamp     = 5, ///< On/off output
    Gauge    = 6, ///< Analog output 0-65535
    Text     = 7, ///< Fixed-length text output
};

/// True for kinds that flow from the cockpit to DCS.
constexpr bool isInputKind(Kind k) {
    return k == Kind::Switch || k == Kind::Selector || k == Kind::Button ||
           k == Kind::Axis || k == Kind::Rotary;
}

/**
 * @brief A catalogue control reference. K and N are part of the type, so a
 *        binding can be checked at compile time (see HornetElements.h).
 *
 * N is the number of positions for switches, selectors and buttons, the text
 * length for text controls, and 0 otherwise.
 */
template <Kind K, uint8_t N>
struct Control {
    uint16_t id; ///< (panel id << 8) | control id
    constexpr explicit Control(uint16_t i) : id(i) {}
    static constexpr Kind kind() { return K; }
    static constexpr uint8_t count() { return N; }
    constexpr uint8_t panel() const { return static_cast<uint8_t>(id >> 8); }
};

// ControlInfo.flags bits
constexpr uint8_t kInfoInput     = 0x01; ///< Cockpit → DCS
constexpr uint8_t kInfoOutput    = 0x02; ///< DCS → cockpit
constexpr uint8_t kInfoGuard     = 0x04; ///< Switch guard / cover
constexpr uint8_t kInfoMagnetic  = 0x08; ///< Electrically held switch
constexpr uint8_t kInfoVerified  = 0x10; ///< Checked in a live cockpit
constexpr uint8_t kInfoNoDcsData = 0x20; ///< No DCS mapping yet

/// One row of the generated flash-resident catalogue table.
struct ControlInfo {
    uint16_t id;
    uint8_t kind;        ///< Kind as uint8_t
    uint8_t count;       ///< positions, or text length
    uint8_t flags;       ///< kInfo* bits
    const char* name;    ///< "PANEL.CONTROL" (flash on AVR)
    const char* positions; ///< "A|B|C" or nullptr (flash on AVR)
};

} // namespace Hornet
