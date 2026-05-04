# Hornet Link — Items Not Completed

This document lists all features and tasks from the original plan that were not
fully implemented during the automated development session, together with notes
on scope and recommended next steps.

---

## 1. MSFS 2024 Source (`MsfsSource.hpp`)

**Status**: Stub only — `connect()` immediately returns `false` with an error message.

**What is missing**:
- SimConnect SDK integration (requires MSFS SDK installation)
- Variable subscription via `SimConnect_AddToDataDefinition`
- Mapping between MSFS L-vars / A-vars and the DCS-BIOS address space
- Import command forwarding via SimConnect event system

**Recommendation**: Install the MSFS 2024 SDK, add a `MSFS_SDK` CMake variable
pointing to the SDK root, link against `SimConnect.lib`, and implement
`MsfsSource::connect()` to open a SimConnect session and start a polling thread.

---

## 2. Unit Test Executable

**Status**: ✅ COMPLETE — 14 unit tests implemented and passing.

**What was done**:
- Created `tests/test_main.cpp` with a custom lightweight test framework (no GoogleTest dependency)
- 6 test suites: BiosProtocol ExportParser (3), HandshakeParser (2), BuildDeltaFrame (2),
  RS485 Protocol/CRC (3), ControlDatabase JSON loading (2), RS485 Bus Auto-Discovery (2)
- `tests/CMakeLists.txt` added; test executable builds to `tests/build/Release/hornet-link-tests.exe`
- All 14/14 tests pass

**To run**:
```
cmake --build tests/build --config Release
tests/build/Release/hornet-link-tests.exe
```

---

## 3. Doxyfile

**Status**: ✅ COMPLETE — `Doxyfile` committed at repository root.

**What was done**:
- Generated and committed `Doxyfile` with:
  - `PROJECT_NAME = "Hornet Link"`
  - `INPUT = Programs/dcsbios-serial-bridge/src libraries/HornetLink/src`
  - `EXTRACT_ALL = YES`, `GENERATE_HTML = YES`, `GENERATE_LATEX = NO`
  - `OUTPUT_DIRECTORY = docs/doxygen`

---

## 4. `device_profiles.json` Seed File

**Status**: ✅ COMPLETE — seed file committed with 8 OpenHornet panel templates.

**What was done**:
- Created `Programs/dcsbios-serial-bridge/device_profiles.json` with default templates
  for 8 OpenHornet panel types (UFC, IFEI, AMPCD left/right, DDI, CMSC, FUEL, ADV)
- `CMakeLists.txt` updated to copy the file to the build output directory

---

## 5. Arduino Library Compile Verification (Local)

**Status**: The CI workflow compiles all three sketches with `arduino-cli`.  This
has not been verified on a local development machine.

**What is missing**:
- Local `arduino-cli` installation
- Core package installation: `arduino:avr`, `esp32:esp32`
- Library dependency check (no external libraries required currently)

**Recommendation**:
```sh
arduino-cli core install arduino:avr
arduino-cli core install esp32:esp32
arduino-cli compile --fqbn arduino:avr:mega sketches/sketch_mega2560_master
arduino-cli compile --fqbn arduino:avr:leonardo sketches/sketch_pro_micro_slave
arduino-cli compile --fqbn esp32:esp32:esp32 sketches/sketch_esp32_master
```

---

## 6. HornetLinkMaster Lambda CRC

**Status**: ✅ COMPLETE — lambda replaced with a static inline helper.

**What was done**:
- Extracted the inline CRC-extending lambda from `writeBusFrame()` into a
  `static` private method `crc16Extend(uint16_t crc, const uint8_t* data, uint8_t len)`
- Method is C++11 compatible; no lambda, no closure, no MSVC C2975 risk
- `writeBusFrame()` now calls `crc16Extend()` directly

---

## 7. C4806 Compiler Warning

**Status**: ✅ COMPLETE — MSVC warning fixed.

**What was done**:
- Fixed three SOCKET_ERROR comparisons in `DcsBiosSource.hpp`
- `bind()`: Changed `== SOCKET_ERROR` → `!= 0`
- `setsockopt()`: Changed `== SOCKET_ERROR` → `!= 0`
- `recv()`: Changed `== SOCKET_ERROR` → `< 0`
- These are semantically equivalent but avoid signed/unsigned comparison warnings

---

## 8. Control Database (`ControlDatabase.hpp`) Population

**Status**: ✅ COMPLETE — `ControlDatabase::loadFromJson(path)` method implemented.

**What was done**:
- Added public `loadFromJson(const std::string& jsonPath)` method
- Loads DCS-BIOS control-reference JSON files into the in-memory database
- Returns count of newly loaded controls (non-destructive append)
- Tested with unit tests for empty database and missing file cases
- Can be called from `BridgeController::Start()` or UI code

**How to use**:
```cpp
ControlDatabase db;
size_t loaded = db.loadFromJson("path/to/FA-18C_hornet.json");
if (loaded > 0) {
    auto ctrl = db.lookupById("UFC_OPTION1");
    // use control descriptor...
}
```

---

## 9. Lua Exporter — Non-DCS-BIOS Path

**Status**: ✅ COMPLETE — DCS-BIOS dependency documented in README.

**What was done**:
- Added a dedicated "Dependencies" section to `README.md` explaining:
  - DCS-BIOS is required for full cockpit panel data
  - Without DCS-BIOS, `HornetLinkExport.lua` falls back to a synthetic heartbeat (altitude at address 0x0000 only)
  - Installation instructions and links
  - Both supported data paths (UDP multicast and direct Lua→UDP)
- Updated Known Limitations to reference the new Dependencies section

---

## 10. RS-485 Bus Auto-Discovery

**Status**: ✅ COMPLETE — full 254-address dynamic discovery implemented.

**What was done**:
- Expanded `kHL_MaxSlaves` from 16 → 254 (full RS-485 slave address range)
- Added `kHL_ScanRetryMs` (5000 ms) and `kHL_KeepaliveMs` (500 ms) constants
- Replaced fixed `slaveAddrs_[16]` array with 32-byte `aliveMap_` bit-field
- `pollBus()` now sweeps all 254 addresses progressively:
  - Dead addresses re-probed every `kHL_ScanRetryMs`
  - Alive slaves polled every `kHL_KeepaliveMs` for keep-alive
- Added helpers: `registerSlave()`, `unregisterSlave()`, `isAlive()`, `aliveCount()`
- `flushToSlaves()` and `onModeChange()` updated to iterate alive bitmap
