# Hornet Link — Agent Reference Guide

Authoritative reference for AI coding agents working in the `router-software` repository.

---

## 1. Repository Layout & Subsystems

```
router-software/
├── AGENTS.md                  # This reference guide for AI agents
├── CONTRIBUTING.md            # Contributor guidelines for human developers
├── REFERENCE.md               # Master reference: work log, complete file catalog, Arduino API
├── README.md                  # Project overview, quickstart, licensing
├── CHANGELOG.md               # Version history and unreleased changes
├── LICENSE                    # CC BY-NC-SA 4.0 license text
├── SECURITY.md                # Security vulnerability reporting policy
├── Doxyfile                   # Doxygen configuration for C++ API documentation
├── catalog/                   # Canonical source-of-truth JSON specifications
│   ├── fa18c.json             # F/A-18C cockpit controls and device mappings
│   └── protocol_v2.json       # Protocol v2 message IDs, wire limits, enums
├── libraries/HornetLink/      # Arduino library (Protocol v1 and v2)
│   ├── library.properties     # Arduino library metadata
│   └── src/                   # C++ headers and implementations
│       ├── generated/         # Code generated from catalog/ (DO NOT EDIT)
│       └── protocol/          # Low-level COBS, framing, and node/master state machines
├── sketches/                  # Example Arduino sketches (Protocol v1 and v2)
├── Programs/
│   ├── dcsbios-serial-bridge/ # Windows C++ GUI bridge application
│   │   ├── src/               # Bridge sources (protocols, sources, UI)
│   │   ├── lua/               # DCS export Lua scripts
│   │   └── templates/         # Default panel subscription templates
│   └── tools/                 # Python code generation and utility scripts
├── tests/                     # C++ unit tests, fuzz tests, test framework
│   ├── arduino_stub/          # Minimal Arduino.h mock for portable C++ testing
│   └── fuzz/                  # libFuzzer targets for parsers and framing
└── docs/                      # Architectural, protocol, and firmware references
```

---

## 2. Critical Constraints & Invariants

### 2.1 Firmware Portability (Arduino / AVR / ARM / ESP32)
- **C++11 Compatibility:** Headers in `libraries/HornetLink/` must stay compatible with C++11 and `avr-gcc 7.3`.
- **Zero Heap Allocation:** No dynamic memory allocation (`malloc`, `free`, `new`, `delete`, `std::vector`, `std::string`) in firmware libraries or sketches.
- **No Virtual Destructors on Base Classes:** Use `protected: ~Base() = default;` instead of `virtual ~Base()`. Virtual destructors pull `operator delete` and heap runtime into AVR binaries.
- **No CTAD:** Do not use C++17 Class Template Argument Deduction in library headers.
- **Platform Agnostic:** Sketches and library headers must build across `arduino:avr:leonardo`, `arduino:avr:mega`, `esp32:esp32:esp32`, and `arduino:mbed_giga:giga`.

### 2.2 Code Generation Pipeline
- `catalog/fa18c.json` and `catalog/protocol_v2.json` are the sole sources of truth for cockpit controls and Protocol v2 message specifications.
- **Never hand-edit generated files:**
  - `libraries/HornetLink/src/generated/HornetF18C.h`
  - `libraries/HornetLink/src/protocol/HnSpec.h`
  - `Programs/dcsbios-serial-bridge/lua/HornetLinkNative.lua`
  - `docs/F18C_CONTROL_REFERENCE.md`
  - `docs/PROTOCOL_V2_MESSAGES.md`
  - `docs/DCS_DATA_PROVENANCE.md`
- Whenever `catalog/*.json` is modified, run:
  ```bash
  python3 Programs/tools/generate_hornet_catalog.py
  ```
- CI validates this with `python3 Programs/tools/generate_hornet_catalog.py --check`.

### 2.3 PC Bridge (Win32 / C++17)
- C++17 application compiled with MSVC (Visual Studio 17 2022 on Windows) or cross-compiled with MinGW-w64 GCC on Linux.
- Windows CI job is pinned to `windows-2022`.
- Input validation: Treat all serial, network, and file input as untrusted. Strictly bounds-check all buffer lengths, frame sizes, and subscription indices.
- Clang-tidy static analysis is currently advisory in CI (`continue-on-error: true`).

### 2.4 Build Artifacts
- Never commit build output (`build/`, `build-tests/`, `build-fuzz/`, `tests/build/`, `.exe`, `.o`, `.a`).
- Keep git working tree clean before committing.

---

## 3. Development, Build, & Test Workflows

### 3.1 C++ Unit Tests (Linux GCC + ASan/UBSan)
```bash
cmake -S tests -B build-tests -DCMAKE_BUILD_TYPE=Debug -DHORNET_LINK_SANITIZE=ON
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

### 3.2 MinGW Cross-Compile Verification (Linux)
```bash
cd Programs/dcsbios-serial-bridge
x86_64-w64-mingw32-g++-posix -std=c++17 -municode -mwindows \
  -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX \
  -Wall -Wextra -Werror -Wno-unknown-pragmas \
  src/main.cpp -o hornet-link.exe -static \
  -lws2_32 -lshlwapi -lcomdlg32
```

### 3.3 Lua Exporter Linting
```bash
cd Programs/dcsbios-serial-bridge/lua
luacheck .
```

### 3.4 Arduino Sketch Compilation Check
```bash
# Verify library can compile for Leonardo/Pro Micro
arduino-cli compile --libraries libraries --fqbn arduino:avr:leonardo sketches/v2_first_panel

# Verify Mega 2560 bus master
arduino-cli compile --libraries libraries --fqbn arduino:avr:mega sketches/v2_bus_master
```

---

## 4. Key References & Documentation Map

- `REFERENCE.md` — Complete file-by-file inventory, condensation log, and full Arduino library function reference.
- `docs/ARCHITECTURE.md` — High-level architecture, data flow diagrams, simulator interfaces.
- `docs/PROTOCOL_REFERENCE.md` — Protocol v1 wire formats, DCS-BIOS frames, handshake, and RS-485 sub-bus specification.
- `docs/PROTOCOL_V2.md` — Protocol v2 (Hornet-native) framing, bus scheduling, COBS, and addressing.
- `docs/DEVELOPER_GUIDE.md` — Engineering status, future backlog, and architectural goals.
- `docs/FIRMWARE_GUIDE.md` — Hardware wiring, board selection, and sketch deployment.
