# Hornet Link — Architecture Overview

## What Is Hornet Link?

**Hornet Link** (`hornet-link.exe`) is a Windows PC application that bridges
the DCS World flight simulator to Open-Hornet physical cockpit panels.  Panels
are Arduino-based boards connected over serial (USB-CDC) or RS-485 sub-bus.

The system replaces the original `dcsbios-serial-bridge.exe` with a more
capable architecture:

- **Multiple simulator back-ends** — DCS-BIOS stream, native Lua UDP export,
  MSFS 2024 (stub), and binary replay files for offline testing.
- **Operating modes** — Sim / Preflight / Maintenance pushed to all devices.
- **RS-485 sub-bus** — one master COM port drives up to 16 slave panels over
  a shared half-duplex RS-485 bus.
- **Profile store** — per-device subscription lists persisted as JSON so panels
  remember their configuration across sessions.
- **Subscription filtering** — each device declares which DCS-BIOS addresses it
  needs; the bridge sends only the relevant bytes, keeping UART traffic low.

---

## High-Level Component Diagram

```
┌────────────────────────────────────────────────────────────────────────┐
│                         DCS World (PC game)                            │
│  ┌──────────────────────────────────┐                                  │
│  │  HornetLinkExport.lua (Lua hook) │ → UDP 127.0.0.1:42002            │
│  └──────────────────────────────────┘                                  │
│                     OR                                                  │
│  DCS-BIOS multicast UDP 239.255.50.10:5010                             │
└──────────────────────┬─────────────────────────────────────────────────┘
                       │
               ┌───────▼────────────────────────────────────────────────┐
               │              hornet-link.exe  (Win32 UI)               │
               │  ┌────────────────────┐   ┌───────────────────────┐   │
               │  │  ISimSource        │   │  BridgeController     │   │
               │  │  ─────────────     │   │  ─────────────────    │   │
               │  │  DcsDirectSource   │   │  • Start / Stop       │   │
               │  │  DcsBiosSource     │   │  • SetMode            │   │
               │  │  ReplayFileSource  │   │  • OnFrameSync        │   │
               │  │  MsfsSource (stub) │   │  • SendImportCommand  │   │
               │  └────────┬───────────┘   └──────────┬────────────┘   │
               │           │ onFrameSync(dirty)        │                 │
               │           │                ┌──────────▼──────────┐     │
               │           │                │  BiosStateMap        │     │
               │           │                │  64 KiB state space  │     │
               │           │                └──────────┬──────────┘     │
               │           │                           │                 │
               │           │                ┌──────────▼──────────┐     │
               │           │                │  DeviceRegistry     │     │
               │           │                │  DeviceInfo list    │     │
               │           │                │  BuildDeltaFrame()  │     │
               │           │                └──────────┬──────────┘     │
               └───────────┴───────────────────────────┼────────────────┘
                                                        │ Serial (Win32)
               ┌────────────────────────────────────────┼────────────────┐
               │          COM Ports                      │                │
               │  ┌──────────────────────────────────────▼──────────┐   │
               │  │  Arduino Mega 2560 (RS-485 Master)              │   │
               │  │  USB-CDC 500 000 baud ← → hornet-link.exe       │   │
               │  │                                                   │   │
               │  │  RS-485 bus 250 000 baud:                        │   │
               │  │    ├── Slave 0x01: UFC Panel (Pro Micro)         │   │
               │  │    ├── Slave 0x02: Master Arm Panel              │   │
               │  │    └── Slave 0x03: Fuel/Engine Panel             │   │
               │  └──────────────────────────────────────────────────┘   │
               │                                                          │
               │  ┌───────────────────────────────────────────────────┐  │
               │  │  Standalone Panel (Pro Micro direct USB)           │  │
               │  └───────────────────────────────────────────────────┘  │
               └──────────────────────────────────────────────────────────┘
```

---

## Source File Map

### PC Bridge (`Programs/dcsbios-serial-bridge/src/`)

| File | Purpose |
|------|---------|
| `main.cpp` | Win32 window, `BridgeController`, `UiState`, entry point |
| `BiosProtocol.hpp` | DCS-BIOS export byte-stream parser (`ExportParser`), `BiosStateMap` |
| `DeviceRegistry.hpp` | `DeviceInfo`, `HandshakeParser`, `BuildDeltaFrame`, `ImportLineParser` |
| `ControlDatabase.hpp` | Address→descriptor lookup table (`ControlDatabase`) |
| `SimSource.hpp` | `ISimSource` abstract interface for pluggable sim back-ends |
| `DcsDirectSource.hpp` | Lua UDP receiver on port 42002 (primary back-end) |
| `DcsBiosSource.hpp` | DCS-BIOS UDP/TCP stream receiver (secondary / compat) |
| `ReplayFileSource.hpp` | Binary capture replay for offline testing |
| `MsfsSource.hpp` | MSFS 2024 stub (not yet implemented) |
| `ProfileStore.hpp` | JSON persistence for per-device subscription lists |
| `RS485ProtocolSpec.hpp` | RS-485 sub-bus frame constants, CRC, encode/verify |

### Firmware (`libraries/HornetLink/src/`)

| File | Purpose |
|------|---------|
| `HornetLinkBase.h` | Shared constants, flags, CRC, pong-frame builder |
| `HornetLinkMode.h` | Mode-frame parse + ack handler (mixin) |
| `HornetLinkImport.h` | Import-command sender (device → DCS) |
| `HornetLinkMaster.h` | RS-485 bus master logic |
| `HornetLinkSlave.h` | RS-485 bus slave logic |
| `HornetLinkCompatDcsBios.h` | DCS-BIOS library compatibility shim |

### Lua (`lua/`)

| File | Purpose |
|------|---------|
| `HornetLinkExport.lua` | DCS export hook — forwards data to port 42002 |
| `modules/FA-18C.lua` | F/A-18C DCS-BIOS address map reference |

---

## Threading Model

```
UI Thread (Win32 message loop)
  │
  ├── Creates BridgeController
  ├── Calls Start() → creates ISimSource, opens COM ports
  └── Handles WM_USER messages (log lines, stopped notification)

ISimSource worker thread (one per source)
  │
  ├── DcsDirectSource: blocks on recvfrom() UDP 42002
  ├── DcsBiosSource:   blocks on recv() multicast or TCP
  ├── ReplayFileSource: reads binary file with timed delays
  │
  └── On each frame:
        ExportParser.processBytes() → updates BiosStateMap
        onFrameSync(dirty) callback →
          BridgeController::OnFrameSync() (called on source thread)
            │
            └── For each COM port in parallel:
                  BuildDeltaFrame() → WriteFile() to serial port
```

The source worker thread and the UI thread both access `BridgeController` fields.
`running_` is `std::atomic<bool>`.  The `stateMap_` array is written exclusively by
the source thread; COM port writes are also on the source thread.  The UI thread only
reads non-atomic fields after `Stop()` returns (join is implicit in `disconnect()`).

---

## Data Flow Summary

1. DCS emits state changes → Lua hook packs them as DCS-BIOS frames → UDP to `127.0.0.1:42002`.
2. `DcsDirectSource` receives datagrams → feeds `ExportParser` → updates `BiosStateMap`.
3. Parser fires `onFrameSync(dirty)` with a list of changed word addresses.
4. `BridgeController::OnFrameSync` iterates serial ports.
5. For each port: `BuildDeltaFrame(stateMap, dirty, device)` filters to subscribed addresses
   and packs a minimal DCS-BIOS wire frame.
6. Frame is written via `WriteFile()` to the COM port.
7. Arduino master board receives the frame and relays it to slave panels via RS-485.
8. Panel firmware extracts subscribed words, updates 7-segment displays / LEDs.
9. (Optionally) firmware sends `SET CONTROL VALUE\n` back over serial.
10. `ImportLineParser` in `DeviceRegistry` fires a callback → `SendImportCommand()` →
    `source_->sendImport()` → UDP to DCS (`127.0.0.1:7778` DCS-BIOS import socket).

---

## Build System

CMake, minimum version 3.15.  Single target `hornet-link` (executable).
Windows-only: links `Ws2_32`, `Comctl32`, `Comdlg32`.

```sh
cd Programs/dcsbios-serial-bridge
cmake -S . -B build
cmake --build build --config Release
# Output: build/Release/hornet-link.exe
```
