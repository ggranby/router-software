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
- `Programs/dcsbios-serial-bridge/lua/` — DCS Lua exporters (DCS-BIOS forwarder and the
  generated Hornet-native exporter).
- `catalog/` — the F/A-18C control catalogue and protocol v2 spec (sources of truth
  for generated code and docs).
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
- A `ProfileStore` component (device-name-based profile lookup, panel
  templates, and atomic persisted overrides) with unit tests. It is **not yet
  wired into the bridge**, and the shipped `src/device_profiles.json` and
  `templates/panels.json` use a different, non-JSON-compliant layout (hex
  literals, top-level `devices`/`panels` arrays) from the documented format the
  loader accepts. See future work item 4.
- Basic source and COM-port preferences persisted beside the executable.
- RS-485 master/slave firmware, dynamic slave discovery, keep-alive handling,
  and mode messages.
- Runtime log-channel controls, bounded UI/log buffers, capture-to-disk, dry-run
  mode, startup options, and operator diagnostics.
- **Hornet-native path (protocol v2), stage 1.** No DCS-BIOS needed at run time:
  - `catalog/fa18c.json` holds every F/A-18C control once, with stable IDs, types,
    position names and DCS data. `Programs/tools/generate_hornet_catalog.py`
    generates the Arduino header, `HnSpec.h`, the native Lua exporter
    (`lua/HornetLinkNative.lua`) and the reference docs from it, plus a
    catalogue hash.
  - Portable protocol core in `libraries/HornetLink/src/protocol/`: COBS
    framing, typed records, node and polled bus master.
  - Arduino API (`Hornet.h`): named elements, IO sources (pins, 74HC165/595,
    MCP23017, matrix, CD4067), filters, ASCII debug mode, five example sketches.
  - Bridge-side logic in `src/HornetNative.hpp`: exporter parser, input
    validation, DCS-is-truth sync tracker, frame decoder, `LinkSession`.
  - Unit tests include a simulated three-slave bus, plus fuzz targets.
  - Docs: [PROTOCOL_V2.md](PROTOCOL_V2.md) and [FIRST_PANEL.md](FIRST_PANEL.md).
  - Not yet wired into `main.cpp`: see future work item 1.
- CMake-based C++ unit tests, libFuzzer targets for the untrusted-input
  parsers, and the CI jobs listed under [Continuous integration](#continuous-integration).

These are implementation facts, not a claim that every hardware combination
or long-running workload has been validated. The test plan includes manual
simulator and hardware scenarios; consult CI and run the relevant hardware
checks before treating a change as release-ready.

## Constraints and known issues

- The bridge is Windows-only and depends on Windows APIs for its GUI, sockets,
  and serial ports.
- The DCS-BIOS UDP source binds `INADDR_ANY:5010` for multicast, so LAN hosts
  can send export frames. Input is bounds-checked; see [SECURITY.md](../SECURITY.md).
- Full DCS cockpit data requires DCS-BIOS until the Hornet-native path is wired
  into the bridge (future work item 1). The DCS-BIOS forwarder's fallback
  heartbeat is only for connectivity checks.
- **DCS data in the catalogue is temporarily taken from DCS-BIOS.** Every
  catalogue entry's DCS device ID, argument, command code and indication field
  is marked `"source": "dcsbios"`. [DCS_DATA_PROVENANCE.md](DCS_DATA_PROVENANCE.md)
  lists every occurrence. Replace them with data read from a DCS install; see
  future work item 2.
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

The test executable exercises the export parser, state map bounds, handshake
(including RS-485 slave lists), import-line validation, delta frames, CRC and
RS-485 framing, wire-format logging, and `ProfileStore`. The core headers used
by the tests are portable, so the same suite also builds on Linux:

```bash
cmake -S tests -B build-tests -DHORNET_LINK_SANITIZE=ON   # ASan + UBSan
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

Fuzz targets live in `tests/fuzz/` and require Clang:

```bash
cmake -S tests -B build-fuzz -DCMAKE_CXX_COMPILER=clang++ -DHORNET_LINK_FUZZ=ON
cmake --build build-fuzz
./build-fuzz/fuzz/fuzz_handshake -max_total_time=60
```

Hardware and simulator integration scenarios are described in
`tests/test-plan.yaml` and require their listed prerequisites.

### Continuous integration

`.github/workflows/ci.yml` runs on changes to sources, tests, docs, and CI
configuration:

| Job | Purpose | Blocking |
|---|---|---|
| `unit-tests` | Tests on Windows / MSVC | Yes |
| `unit-tests-linux` | Tests on Linux / GCC with ASan + UBSan | Yes |
| `fuzz` | Each libFuzzer target for 60 s; crash inputs uploaded as artifacts | Yes |
| `build-mingw` | MinGW-w64 cross-compile of the bridge with `-Werror` | Yes |
| `lua-lint` | `luacheck` on the Lua exporter (`lua/.luacheckrc`) | Yes |
| `build-windows` | MSVC Release build and artifact; needs both test jobs | Yes |
| `static-analysis` | `clang-tidy` with the root `.clang-tidy` | Advisory |
| `arduino-build` | Mega 2560, Pro Micro, and ESP32 sketches | Yes |
| `docs` | Doxygen from the root `Doxyfile`; warning count reported | Advisory |

Third-party actions are pinned by commit SHA and kept current by Dependabot.

### Versioning

The bridge version is `project(hornet_link VERSION x.y.z)` in
`Programs/dcsbios-serial-bridge/CMakeLists.txt`. It feeds the window title, the
Windows VERSIONINFO resource, and CPack package names; the release workflow
fails if the pushed tag is not `v<version>`. The Arduino library is versioned
separately in `libraries/HornetLink/library.properties`.

### Arduino checks

CI compiles the Mega 2560 master, Pro Micro slave, and ESP32 master sketches
with `arduino-cli`. It also compiles the protocol v2 sketches (`sketches/v2_*`)
for Leonardo/Pro Micro, Mega 2560, ESP32 and Arduino Giga R1. The `catalog`
job runs `python3 Programs/tools/generate_hornet_catalog.py --check`; re-run the
generator without `--check` after editing anything in `catalog/`. When changing shared firmware code, run those board builds
or verify the corresponding CI job. The exact CI commands and board targets are
in `.github/workflows/ci.yml`.

## Support matrix

| Component | Supported | Verified in CI |
|---|---|---|
| Bridge OS | Windows 10/11 x64 | Build only (MSVC, MinGW) |
| Bridge toolchain | MSVC 2022 (VS 17), CMake ≥ 3.20 | Yes |
| Core headers / tests | MSVC, GCC, Clang (C++17) | Yes |
| Firmware boards | Arduino Mega 2560, Pro Micro (Leonardo), ESP32; Arduino Giga R1 (v2 only) | Compile only |
| v2 library RAM budget | Pro Micro (2.5 KB RAM) is the smallest panel target; a bus master needs a Mega/ESP32/Giga | Compile only |
| Lua exporter | DCS World export environment (Lua 5.1) | Lint only |

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

### 1. Finish the Hornet-native path in the bridge (stage 2)

Stage 1 (catalogue, generator, protocol v2, Arduino library, native exporter
and the bridge's portable `HornetNative.hpp`) is complete and unit-tested.
What's left is connecting it to the bridge program:

- A native data source in `main.cpp`: receive exporter datagrams on UDP 42003
  into `CatalogState`, and send validated inputs to 42004 with
  `formatExporterInput`.
- COM-port negotiation: `LinkSession::start()` first, and fall back to the v1
  ping if no v2 frame arrives within about 200 ms. Then run v1 and v2 side by
  side.
- UI: nodes by name, the catalogue-mismatch warning, a decoder view using
  `decodeFrame`, mode buttons including wiring test, and the sync overlay
  on/off setting.
- In-game discrepancy overlay: a `Scripts/Hooks` GUI script that shows
  `SyncTracker::overlayText()`.
- Pass HELLO board IDs to item 4 (board identity).
- Retire v1 once v2 is verified on real panels.

**Done when:** a USB panel and a bus master with slaves run through the bridge
against DCS using only `HornetLinkNative.lua`. The hardware-only scenarios in
`tests/test-plan.yaml` (HN-*) pass.

### 2. Replace DCS-BIOS-derived DCS data in the catalogue

Every `dcs` block in `catalog/fa18c.json` is currently copied from DCS-BIOS
(`"source": "dcsbios"`). The register is
[DCS_DATA_PROVENANCE.md](DCS_DATA_PROVENANCE.md). Extract the device IDs,
argument numbers, command codes and indication fields from the DCS install
(`Mods/aircraft/FA-18C/Cockpit/Scripts/clickabledata.lua`, `devices.lua`,
`command_defs.lua`, and the display indication scripts). Check each value in
the cockpit, set `"source": "dcs"`, and regenerate.

**Done when:** no catalogue entry has `"source": "dcsbios"` and the provenance
register is empty.

### 3. Validate long-session behavior and UI status reporting

The code already has log-channel controls, bounded buffers, capture-to-disk,
and runtime metrics. The remaining task is to validate them under representative
loads and resolve the reported status-text flicker if it is reproducible.

**Done when:** idle, switch-heavy, knob-heavy, and gauge-heavy sessions complete
without UI freezes or unbounded queue growth; dropped lines and capture output
are understood; any confirmed flicker has a regression check or documented
resolution.

### 4. Add stable board identity and explicit panel assignment

First wire `ProfileStore` into the bridge and reconcile the shipped
`device_profiles.json` / `templates/panels.json` with the format the loader
accepts (documented in `ProfileStore.hpp` and covered by
`tests/test_profile_store.cpp`). Then define which identity fields are available
and reliable, track reconnects and port renumbering, and provide user
confirmation when a board-to-panel match is ambiguous. Keep profile-by-name
lookup as a fallback. Protocol v2 HELLO already carries an 8-byte board ID,
firmware version and catalogue hash, and DESCRIBE lists the controls each
board owns. Use these as the identity fields.

**Done when:** users can review and persist board-to-panel assignments, restore
them after reconnects or port changes when identity is reliable, and resolve
uncertain matches explicitly without silent remapping.

### 5. Decide and implement an MSFS integration

Choose an integration API and establish how MSFS variables map to device
subscriptions and import commands before implementing the source. Avoid
silently treating MSFS variables as DCS-BIOS addresses.

**Done when:** the chosen API, SDK/dependency requirements, mapping model, and
supported variables are documented; the source reports connection failures
clearly, updates state through the bridge, and has tests for both state and
import paths.

### 6. Reassess hardware abstraction

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
