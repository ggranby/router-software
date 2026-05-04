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

**Status**: `tests/test-plan.yaml` is a human-readable test specification document,
not a runnable test suite.

**What is missing**:
- A compilable C++ test executable (e.g. using GoogleTest or Catch2)
- Tests for `HandshakeParser`, `ExportParser`, `BuildDeltaFrame`, `crc16CcittFalse`,
  and `RS485Frame::verifyCrc`
- CMake `enable_testing()` integration so `ctest` can run them

**Recommendation**: Add a `tests/` CMake subdirectory, fetch GoogleTest via
`FetchContent`, and implement each case listed in `test-plan.yaml` as a `TEST()`.

---

## 3. Doxyfile

**Status**: The CI `docs` job runs `doxygen -g` to generate a default Doxyfile
on-the-fly.  A committed `Doxyfile` does not exist in the repository.

**What is missing**:
- A committed `Doxyfile` at the repository root configured with:
  - `PROJECT_NAME = "Hornet Link"`
  - `INPUT = Programs/dcsbios-serial-bridge/src libraries/HornetLink/src`
  - `EXTRACT_ALL = YES`
  - `GENERATE_HTML = YES`, `GENERATE_LATEX = NO`
  - `OUTPUT_DIRECTORY = docs/doxygen`
- Doxygen `///` comment blocks on all public methods in the HPP headers

**Recommendation**: Run `doxygen -g Doxyfile`, edit the file as above, add doc
comments to the headers, commit, then verify the CI `docs` job succeeds.

---

## 4. `device_profiles.json` Seed File

**Status**: `ProfileStore` loads `device_profiles.json` at startup but no initial
file is distributed with the repository.  First-run users start with no profiles.

**What is missing**:
- A seed `device_profiles.json` containing OpenHornet panel defaults
- The mapping from panel display names (returned in the handshake) to panel IDs
  in `templates/panels.json`

**Recommendation**: Populate the file by running the bridge against real hardware,
copying the auto-generated file from the executable directory, and committing it
to `Programs/dcsbios-serial-bridge/`.  Update `CMakeLists.txt` to copy it to the
build output directory.

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

**Status**: `HornetLinkMaster.h` uses a C++14 lambda inside a member initializer.
This may not compile on older MSVC versions if the library is ever used in a
non-Arduino context (e.g. via PlatformIO on MSVC).

**What is missing**:
- A verification compile on PlatformIO + MSVC toolchain
- Possibly replacing the lambda with a plain inline member function

**Recommendation**: Test via PlatformIO; if the lambda causes a C2975 error,
extract it to a `static` inline helper method.

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

**Status**: `HornetLinkExport.lua` falls back to a synthetic heartbeat frame
(address 0x0000) when DCS-BIOS is not installed.  This means the bridge receives
no useful cockpit data unless DCS-BIOS is present.

**What is missing**:
- A pure-Lua address extraction path using `LoGetNameByType` / `LoGetAircraftDrawArgumentValue`
  for at least the F/A-18C's most important indicators
- Or: documentation explicitly stating that DCS-BIOS is required

**Recommendation**: Either document the DCS-BIOS dependency clearly in the README,
or implement a partial Lua-native data path for the fields listed in
`lua/modules/FA-18C.lua`.

---

## 10. RS-485 Bus Auto-Discovery

**Status**: The master firmware sketch uses hardcoded slave address ranges
(polling 1–15).  There is no dynamic discovery beyond the addresses declared
in the handshake pong.

**What is missing**:
- A bus scan command that probes all 254 possible slave addresses at startup
- Caching of discovered slaves into the profile store
- A UI indicator for newly-discovered slaves

**Recommendation**: Implement a scan loop in `HornetLinkMaster::scanBus()` using
the existing `kSubBusMsgProbe` frame, called once after handshake ACK.
