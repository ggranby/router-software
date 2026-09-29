# Hornet Link — Development Guide

This is the authoritative guide to the project's current development status,
supported workflows, known constraints, and future work. It replaces the
older status and incomplete-items inventories. Protocol details and API
contracts remain in the linked reference documents.

## Project at a glance

Hornet Link is a Windows C++ application and Arduino library that carry DCS
cockpit data to physical panels. The PC bridge receives DCS-BIOS data via UDP
or TCP, direct Lua-exported UDP, or a binary replay; parses it into simulator
state; and sends subscribed updates to serial-connected devices. Devices can
forward control inputs back to DCS, and an RS-485 master can serve downstream
panels.

The repository contains:

- `Programs/dcsbios-serial-bridge/` — Windows GUI bridge and simulator sources.
- `libraries/HornetLink/` and `sketches/` — Arduino library and example firmware.
- `Programs/dcsbios-serial-bridge/lua/` — DCS Lua exporter and aircraft module.
- `tests/` — C++ protocol and bridge unit tests, plus a manual integration plan.
- `docs/` — architecture, protocol, API, firmware, and release references.

For component relationships and data flow, see [ARCHITECTURE.md](ARCHITECTURE.md).
Wire formats are described in [PROTOCOL_REFERENCE.md](PROTOCOL_REFERENCE.md).

## Current implementation

The following capabilities are present in the repository:

- Windows bridge with DCS-BIOS UDP multicast/TCP, direct Lua UDP, and replay
  sources; MSFS is a placeholder, not a working source.
- Serial multi-port output, device handshake, subscription filtering, and
  bidirectional import commands.
- Device-name-based profile lookup, built-in panel templates, and persisted
  per-device profile overrides.
- Basic source and COM-port preferences persisted beside the executable.
- RS-485 master/slave firmware, dynamic slave discovery, keep-alive handling,
  and mode messages.
- Runtime log-channel controls, bounded UI/log buffers, capture-to-disk, dry-run
  mode, startup options, and operator diagnostics.
- CMake-based C++ unit tests and CI jobs for the Windows app, Arduino sketches,
  static analysis, and documentation generation.

These are implementation facts, not a claim that every hardware combination
or long-running workload has been validated. The test plan includes manual
simulator and hardware scenarios; consult CI and run the relevant hardware
checks before treating a change as release-ready.

## Constraints and known issues

- The bridge is Windows-only and depends on Windows APIs for its GUI, sockets,
  and serial ports.
- Full DCS cockpit data requires DCS-BIOS. The Lua exporter's fallback heartbeat
  is only for connectivity checks.
- `MsfsSource` is a stub. MSFS integration needs a deliberate variable/address
  mapping design as well as simulator API integration; the DCS-BIOS 64 KiB
  address map cannot be assumed to represent MSFS variables.
- Profiles currently rely on the device-reported name. Stable board identity,
  reconnect tracking across COM-port renumbering, and a full board-to-panel
  assignment workflow are not implemented.
- A UI status-text flicker under high log throughput is listed in
  `Programs/dcsbios-serial-bridge/known-bugs.md`. Reproduce and verify it before
  treating the report as a current regression.

## Build and test

### Requirements

- Windows 10 or later
- Visual Studio 2022 / MSVC C++ build tools
- CMake 3.20 or later

### Build the bridge

From `Programs/dcsbios-serial-bridge/`:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The executable is `build/Release/hornet-link.exe`. The optional ImGui prototype
is not the production UI and requires the separately provisioned ImGui source.

### Build and run C++ tests

From the repository root:

```powershell
cmake -S tests -B tests/build
cmake --build tests/build --config Release
ctest --test-dir tests/build -C Release --output-on-failure
```

The test executable exercises parser, handshake, frame generation, and RS-485
protocol behavior. CI runs these same steps in the `unit-tests` job, and the
Windows app build only runs after the tests pass. Hardware and simulator
integration scenarios are described in `tests/test-plan.yaml` and require their
listed prerequisites.

### Arduino checks

CI compiles the Mega 2560 master, Pro Micro slave, and ESP32 master sketches
with `arduino-cli`. When changing shared firmware code, run those board builds
or verify the corresponding CI job. The exact CI commands and board targets are
in `.github/workflows/ci.yml`.

## Working on the project

- Keep the UI, source/transport, protocol, and firmware responsibilities clear;
  use [ARCHITECTURE.md](ARCHITECTURE.md) before changing cross-component flow.
- Preserve existing wire formats unless a protocol change is intentional.
  Update [PROTOCOL_REFERENCE.md](PROTOCOL_REFERENCE.md), firmware, and tests
  together when changing one.
- Add or update automated tests for protocol and state-processing behavior.
  Mark hardware-only verification explicitly rather than implying it is covered
  by unit tests.
- Keep operator-facing setup and release instructions aligned with
  [README.md](../README.md) and [RELEASE_PROCESS.md](RELEASE_PROCESS.md).

## Future work

This is the single prioritized backlog. Items are proposals, not commitments or
release dates; adjust priority when requirements or hardware evidence change.

### 1. Validate long-session behavior and UI status reporting

The code already has log-channel controls, bounded buffers, capture-to-disk,
and runtime metrics. The remaining task is to validate them under representative
loads and resolve the reported status-text flicker if it is reproducible.

**Done when:** idle, switch-heavy, knob-heavy, and gauge-heavy sessions complete
without UI freezes or unbounded queue growth; dropped lines and capture output
are understood; any confirmed flicker has a regression check or documented
resolution.

### 2. Add stable board identity and explicit panel assignment

Build on the existing device-name profiles rather than duplicating them. Define
which identity fields are available and reliable, track reconnects and port
renumbering, and provide user confirmation when a board-to-panel match is
ambiguous. Keep current profile-by-name behavior working as a fallback.

**Done when:** users can review and persist board-to-panel assignments, restore
them after reconnects or port changes when identity is reliable, and resolve
uncertain matches explicitly without silent remapping.

### 3. Decide and implement an MSFS integration

Choose an integration API and establish how MSFS variables map to device
subscriptions and import commands before implementing the source. Avoid
silently treating MSFS variables as DCS-BIOS addresses.

**Done when:** the chosen API, SDK/dependency requirements, mapping model, and
supported variables are documented; the source reports connection failures
clearly, updates state through the bridge, and has tests for both state and
import paths.

### 4. Reassess hardware abstraction

After board identity and panel assignment requirements are understood, decide
whether configurable serial framing or support for additional hardware is
needed. Avoid adding speculative transports or configuration layers before
there is a concrete device requirement.

**Done when:** supported hardware and compatibility behavior are documented,
and each added adapter has a testable contract and appropriate hardware checks.

## Related references

- [API_REFERENCE.md](API_REFERENCE.md) — public C++ types and interfaces.
- [FIRMWARE_GUIDE.md](FIRMWARE_GUIDE.md) — Arduino library and sketches.
- [RELEASE_PROCESS.md](RELEASE_PROCESS.md) — packaging and publishing.
- [RESUME_GUIDE.md](RESUME_GUIDE.md) — short handoff entry point.
