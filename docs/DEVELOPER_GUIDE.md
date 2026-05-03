# Hornet Link — Developer Guide

This guide covers how to build, extend, and debug the Hornet Link PC bridge
(`hornet-link.exe`).

---

## Prerequisites

| Tool | Minimum version | Notes |
|------|-----------------|-------|
| Visual Studio 2022 | 17.x | MSVC toolchain; C++17 |
| CMake | 3.15 | Bundled with VS2022 |
| Git | any | For cloning |
| Python 3 | 3.8+ | Optional — `tools/connect-logger.py` |

---

## Building

```powershell
cd Programs\dcsbios-serial-bridge
cmake -S . -B build
cmake --build build --config Release
# Executable: build\Release\hornet-link.exe
```

For a Debug build (enables ASAN on MSVC 2022):

```powershell
cmake --build build --config Debug
```

---

## Project Layout

```
Programs/dcsbios-serial-bridge/
├── CMakeLists.txt          — build definition
├── src/
│   ├── main.cpp            — Win32 UI + BridgeController orchestrator
│   ├── BiosProtocol.hpp    — DCS-BIOS export parser + BiosStateMap
│   ├── DeviceRegistry.hpp  — device metadata, handshake, delta builder
│   ├── ControlDatabase.hpp — address→descriptor lookup
│   ├── SimSource.hpp       — ISimSource interface
│   ├── DcsDirectSource.hpp — Lua UDP back-end (port 42002)
│   ├── DcsBiosSource.hpp   — DCS-BIOS UDP/TCP back-end
│   ├── ReplayFileSource.hpp— binary replay back-end
│   ├── MsfsSource.hpp      — MSFS 2024 stub
│   ├── ProfileStore.hpp    — JSON device profile persistence
│   └── RS485ProtocolSpec.hpp — RS-485 constants + CRC + frame helpers
├── templates/
│   └── panels.json         — built-in OpenHornet panel templates
└── build/                  — CMake output (git-ignored)
```

---

## Adding a New Simulator Back-End

1. Create a new header `src/MySimSource.hpp`.
2. Include `SimSource.hpp` and implement `ISimSource`:

```cpp
class MySimSource : public ISimSource {
public:
    bool connect(BiosStateMap& stateMap, HWND hwnd) override { ... }
    void disconnect() override { ... }
    bool isConnected() const override { return running_; }
    void sendImport(const ImportCommand& cmd) override { ... }
    std::wstring displayName() const override { return L"My Source"; }
    // onFrameSync is a std::function<void(dirty)> field — set it in connect().
};
```

3. Add a `SimSourceType::MySource` enum value in `main.cpp`.
4. Add a `case SimSourceType::MySource:` in `BridgeController::Start()`.
5. Add a combo-box entry in the `WM_CREATE` handler and a matching `case` in `ToggleBridge()`.
6. Add the header to `CMakeLists.txt` sources list.

---

## Adding a New Panel Template

Edit `templates/panels.json`.  Each entry in `"panels"` has:

```json
{
  "id": "MY_PANEL",
  "name": "My Panel",
  "description": "...",
  "subscriptions": [
    { "addr": 4096, "mask": 65535, "shift": 0, "label": "SOME_FIELD" }
  ]
}
```

`addr` is decimal.  The template is automatically loaded by `ProfileStore` as a
fallback when no user override exists for a matching device name.

---

## Logging Architecture

All log output goes through `PostLog(hwnd, text)` which posts a `WM_USER`
message to the UI thread.  The UI thread appends to the read-only `EDIT` control.

Log verbosity is controlled by four `std::atomic<bool>` flags in `BridgeController`:

| Flag | Default | Controls |
|------|---------|---------|
| `liveLogChanges_` | off | Per-frame dirty-address lines |
| `liveLogRawKnobs_` | on | Raw knob/dial import commands |
| `liveLogRawGauges_` | off | Raw gauge analogue values |
| `liveLogDiagnostics_` | off | Connection events, reconnects |

Call `SetLoggingFlags(changes, knobs, gauges, diagnostics)` from the UI thread to
update all four atomically.

---

## Capture and Replay

To record a session for replay:

1. Click **Start Capture** in the UI.  A binary file is created in the executable
   directory with a timestamp name (`capture_YYYYMMDD_HHMMSS.bin`).
2. Click **Stop Capture** to flush and close the file.
3. To replay: select **Replay File…** from the Source combo, click **Start**,
   and choose the `.bin` file.

Binary format: `[timestamp_ms:u32LE][payload_len:u16LE][payload:N]`.

---

## COM Port Auto-Detection

`DetectAvailableComPorts()` in `main.cpp` enumerates `HKEY_LOCAL_MACHINE\HARDWARE\
DEVICEMAP\SERIALCOMM` and returns all present port names.  The result populates
the ports field when **Auto Detect** is clicked or when the ports field is empty
at bridge start.

---

## Metrics and Performance

`BridgeController` tracks two `OnlineAverage` metrics:

- **Dispatch latency** — wall-clock time from `onFrameSync()` entry to last
  `WriteFile()` completion per frame.
- **Port write latency** — per-port `WriteFile()` wall-clock time.

These are logged periodically when `liveLogDiagnostics_` is on.  Target dispatch
latency is < 2 ms per frame at 30 Hz.

---

## Known Limitations and Future Work

- `MsfsSource` is a stub — no SimConnect or FSUIPC integration yet.
- The Lua exporter (`HornetLinkExport.lua`) intercepts `ExportReceiveData` which
  is only available when DCS-BIOS is also installed.  The fallback synthetic frame
  at address 0x0000 is a heartbeat only and carries no useful cockpit data.
- `ControlDatabase` is populated from a JSON file that must match the DCS-BIOS
  `control_reference.json` format.  Distribution of that file is not included in
  this repository.
- RS-485 bus probe uses a fixed 100 ms round-trip poll.  Future work: adaptive
  polling based on observed slave response times.

---

## Debugging Tips

- **No data arriving**: Check that `HornetLinkExport.lua` is sourced from `Export.lua`
  and that port 42002 is not blocked by a firewall.
- **COM port won't open**: Another process (e.g. Arduino IDE serial monitor) may
  hold the port.  Use `mode COMX` in cmd.exe to verify port existence.
- **Handshake not completing**: Enable **Log Diagnostics** to see probe/pong timing.
  If no pong arrives within 300 ms the device is treated as legacy.
- **High dispatch latency**: Check for `WriteFile()` blocking.  Try reducing
  the COM port write timeout (constant `kSerialWriteTimeoutMs = 50` in `main.cpp`).
