# Hornet Link — Master Reference & Work Log

This singular reference document tracks repository cleanup and condensation, provides an exhaustive file-by-file directory breakdown (what each file contains, does, and why), catalogs every Arduino library function and its use, and maps development workflows for human contributors and AI agents.

---

## Table of Contents

1. [Work Tracking & Condensation Register](#1-work-tracking--condensation-register)
   - [What Was Removed](#what-was-removed)
   - [What Was Condensed](#what-was-condensed)
   - [What Stays & Organizational Boundaries](#what-stays--organizational-boundaries)
2. [Complete File-by-File Inventory](#2-complete-file-by-file-inventory)
   - [Root Configuration & Documentation](#root-configuration--documentation)
   - [Control Catalogues & Code Generation (`catalog/`, `Programs/tools/`)](#control-catalogues--code-generation)
   - [Documentation (`docs/`)](#documentation-docs)
   - [Arduino Firmware Library (`libraries/HornetLink/`)](#arduino-firmware-library-librarieshornetlink)
   - [Example Firmware Sketches (`sketches/`)](#example-firmware-sketches-sketches)
   - [Windows Serial Bridge (`Programs/dcsbios-serial-bridge/`)](#windows-serial-bridge-programsdcsbios-serial-bridge)
   - [Test Suite & Quality Verification (`tests/`)](#test-suite--quality-verification-tests)
3. [Arduino Library Function Catalog & Reference](#3-arduino-library-function-catalog--reference)
   - [Protocol v2 High-Level Panel API (`HornetPanel.h`)](#protocol-v2-high-level-panel-api-hornetpanelh)
   - [Cockpit Element Classes (`HornetElements.h`)](#cockpit-element-classes-hornetelementsh)
   - [Hardware Input/Output Drivers (`HornetIO.h`, `HornetMcp23017.h`)](#hardware-inputoutput-drivers-hornetioh-hornetmcp23017h)
   - [Signal Conditioning & Math Filters (`HornetFilters.h`)](#signal-conditioning--math-filters-hornetfiltersh)
   - [Catalogue Types & Lookups (`HornetTypes.h`, `HornetCatalog.h`, `HornetF18C.h`)](#catalogue-types--lookups-hornettypesh-hornetcatalogh-hornetf18ch)
   - [Protocol v2 Framing & Bus Engines (`protocol/Hn*.h`)](#protocol-v2-framing--bus-engines-protocolhnh)
   - [Protocol v1 Legacy Sub-Bus & DCS-BIOS Bridge (`HornetLink*.h`)](#protocol-v1-legacy-sub-bus--dcs-bios-bridge-hornetlinkh)
4. [Agent & Contributor Document Navigation](#4-agent--contributor-document-navigation)

---

## 1. Work Tracking & Condensation Register

### What Was Removed
| File Removed | Reason for Removal |
|---|---|
| `RS485_PROTOCOL.md` (root) | Redundant protocol reference at the repository root. Completely consolidated into `docs/PROTOCOL_REFERENCE.md` (Section 5). |
| `DEVELOPMENT.md` (root) | 7-line redirect stub pointing to `docs/DEVELOPER_GUIDE.md`. Redundant. |
| `docs/INCOMPLETE_ITEMS.md` | 5-line obsolete status stub pointing to `docs/DEVELOPER_GUIDE.md`. Redundant. |
| `docs/RESUME_GUIDE.md` | 12-line handoff stub pointing to `docs/DEVELOPER_GUIDE.md`. Redundant. |
| `Programs/connect-serial-port.cmd` | Legacy batch script relying on external `socat`. Superseded by `Programs/dcsbios-serial-bridge/`. |
| `Programs/ensure-socat.cmd` | Obsolete batch script attempting to unpack a non-existent `socat.zip`. Superseded. |
| `Programs/multicast-console.cmd` | Obsolete batch script running `socat` for UDP multicast console. Superseded. |
| `Programs/multiple-com-ports.cmd` | Obsolete batch script running multiple `connect-serial-port.cmd` loops. Superseded. |

### What Was Condensed
- **Wire Protocol Documentation:** `RS485_PROTOCOL.md` was unified with `docs/PROTOCOL_REFERENCE.md`. All specifications for the physical layer, baud rate (250,000 baud 8N1), transceiver direction pin handling, STX length-prefixed framing, Kermit/CCITT-FALSE CRC-16, probe keep-alives, DCS-BIOS delta writes, ASCII import commands, mode management, timing timeouts, and error handling are now in `docs/PROTOCOL_REFERENCE.md`.
- **Developer & Contributor Guidance:** Contributor instructions are consolidated in `CONTRIBUTING.md`, while engineering status and the feature backlog remain in `docs/DEVELOPER_GUIDE.md`. AI agent operational rules and technical constraints are consolidated in `AGENTS.md`.

### What Stays & Organizational Boundaries
1. **Arduino Library (`libraries/HornetLink/`):** All Arduino library files remain strictly grouped together under `libraries/HornetLink/src/` with subdirectories `protocol/` and `generated/`.
2. **Bridge Software (`Programs/dcsbios-serial-bridge/`):** All PC bridge C++ sources, Lua scripts, packaging batch files, and UI templates remain grouped together under `Programs/dcsbios-serial-bridge/`.
3. **Tools & Generators (`Programs/tools/`):** Code generation scripts (`generate_hornet_catalog.py`, `generate_f18c_inventory.py`), test tools (`connect-logger.py`), and exporter templates remain together in `Programs/tools/`.
4. **Firmware Sketches (`sketches/`):** Ready-to-flash example sketches for Protocol v1 and Protocol v2 panels across Pro Micro, Mega 2560, ESP32, and Arduino Giga R1 remain together in `sketches/`.
5. **Catalogue Sources (`catalog/`):** Machine-readable single sources of truth (`fa18c.json`, `protocol_v2.json`) remain in `catalog/`.
6. **Tests (`tests/`):** Unit tests, mock Arduino stubs, and libFuzzer suites remain in `tests/`.
7. **Documentation (`docs/` and root):** Clear, non-redundant architectural, protocol, API, and firmware documentation.

---

## 2. Complete File-by-File Inventory

Every file in the repository is cataloged below with its contents, purpose, and rationale.

### Root Configuration & Documentation

#### `.clang-format`
- **Contains:** Indentation (4 spaces), column limits, brace wrapping, and naming style rules for C++ code.
- **Does:** Configures automatic code formatters (clang-format) for PC bridge and library sources.
- **Why:** Maintains consistent formatting across C++ code without causing unnecessary diff noise.

#### `.clang-tidy`
- **Contains:** Static analysis check definitions (`modernize-*`, `bugprone-*`, `cert-*`, `clang-analyzer-*`) and header filters.
- **Does:** Directs `clang-tidy` in CI to detect memory leaks, uninitialized variables, and suboptimal constructs.
- **Why:** Provides advisory static analysis in CI to improve bridge code reliability.

#### `.editorconfig`
- **Contains:** Cross-editor file formatting settings (charset UTF-8, LF line endings, 4-space indent for C/C++/Python/JSON, 2-space for YAML).
- **Does:** Normalizes editor whitespace behavior across VS Code, Visual Studio, Vim, and CLion.
- **Why:** Prevents formatting discrepancies across operating systems.

#### `.gitignore`
- **Contains:** Rules ignoring build outputs (`build/`, `build-*/`, `tests/build/`), binaries (`*.exe`, `*.bin`), IDE caches (`.vs/`, `.vscode/`), and Doxygen outputs (`docs/doxygen/`).
- **Does:** Prevents transient artifacts and compiler outputs from being tracked in git.
- **Why:** Keeps the repository tree clean and PR diffs minimal.

#### `AGENTS.md`
- **Contains:** Operational rules, architectural constraints (AVR C++11 safety, no heap), build/test commands, and code generation procedures for AI coding agents.
- **Does:** Serves as the authoritative quick-reference handbook for AI agents working in this repository.
- **Why:** Ensures AI agents work safely within the repository's strict firmware and build constraints.

#### `CHANGELOG.md`
- **Contains:** Chronological release history and an `[Unreleased]` staging section following Keep a Changelog conventions.
- **Does:** Documents features, improvements, fixes, and breaking changes for each version.
- **Why:** Provides human contributors and users with clear change tracking across versions.

#### `CONTRIBUTING.md`
- **Contains:** Contributor guidelines, development environment setup, local build and test commands, style rules, and PR workflow.
- **Does:** Guides human developers on how to fork, develop, test, and submit contributions.
- **Why:** Standardizes the contribution workflow for open-source community contributors.

#### `Doxyfile`
- **Contains:** Doxygen configuration for extracting code documentation from `Programs/dcsbios-serial-bridge/src` and `libraries/HornetLink/src`.
- **Does:** Generates HTML API documentation for public C++ types and functions.
- **Why:** Enables automated documentation builds verified in CI.

#### `LICENSE`
- **Contains:** Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0) license text.
- **Does:** Governs the distribution, modification, and use of this software.
- **Why:** Protects author attribution while allowing non-commercial community sharing and OpenHornet compatibility.

#### `README.md`
- **Contains:** Project overview, key features (dual protocol, multi-port fanout, dry-run mode), OpenHornet compatibility notes, and quickstart build instructions.
- **Does:** Serves as the front-page introduction for visitors and users of the repository.
- **Why:** Explains what the software does and how to get started using it.

#### `REFERENCE.md`
- **Contains:** This comprehensive master reference file, including work tracking, complete file directory breakdown, and Arduino library API function catalog.
- **Does:** Serves as a singular lookup reference for all files and functions in the repository.
- **Why:** Fulfills the requirement for a consolidated repository reference.

#### `SECURITY.md`
- **Contains:** Vulnerability disclosure policy, supported versions, and reporting instructions.
- **Does:** Instructs security researchers on how to report potential vulnerabilities responsibly.
- **Why:** Ensures sensitive security issues are handled safely before public disclosure.

---

### Control Catalogues & Code Generation

#### `catalog/fa18c.json`
- **Contains:** Structured JSON catalog of every F/A-18C cockpit control, switch positions, rotary step values, display lines, and DCS device/argument bindings.
- **Does:** Acts as the primary source of truth for the F/A-18C cockpit controls across the entire system.
- **Why:** Decouples cockpit control definitions from hand-written code, allowing automated code and doc generation.

#### `catalog/protocol_v2.json`
- **Contains:** Machine-readable Protocol v2 specification, including message type IDs, wire limits (header bytes, CRC size, payload limit), address constants, and enums.
- **Does:** Acts as the single source of truth for the Hornet-native Protocol v2 wire specification.
- **Why:** Eliminates manual synchronization errors between C++ firmware, C++ bridge, Lua exporter, and documentation.

#### `Programs/tools/generate_hornet_catalog.py`
- **Contains:** Python script that parses `catalog/fa18c.json` and `catalog/protocol_v2.json`.
- **Does:** Automatically generates `HornetF18C.h`, `HnSpec.h`, `HornetLinkNative.lua`, `F18C_CONTROL_REFERENCE.md`, `PROTOCOL_V2_MESSAGES.md`, and `DCS_DATA_PROVENANCE.md`. Supports a `--check` flag.
- **Why:** Guarantees that C++ headers, Lua exporters, and markdown docs stay strictly synchronized with the catalogue.

#### `Programs/tools/generate_f18c_inventory.py`
- **Contains:** Python script that ingests raw DCS-BIOS reference JSON files and outputs `docs/F18C_EXPORT_INVENTORY.md`.
- **Does:** Generates an inventory comparing DCS-BIOS addresses to native cockpit control metadata.
- **Why:** Tracks compatibility and address provenance when importing DCS-BIOS definitions.

#### `Programs/tools/connect-logger.py`
- **Contains:** Python utility for connecting to serial ports and logging raw frames to disk or terminal.
- **Does:** Provides interactive serial inspection and diagnostic logging.
- **Why:** Assists developers in diagnosing serial protocol traffic during board bring-up.

#### `Programs/tools/templates/HornetLinkNative.lua.in`
- **Contains:** Template file used by `generate_hornet_catalog.py` to produce `HornetLinkNative.lua`.
- **Does:** Defines the Lua export harness that hooks into DCS World's `Export.lua` pipeline.
- **Why:** Allows DCS export code to be generated programmatically from the control catalog.

---

### Documentation (`docs/`)

#### `docs/API_REFERENCE.md`
- **Contains:** C++ API documentation for core PC bridge classes (`BiosStateMap`, `ExportParser`, `HandshakeParser`, `ISimSource`, `ProfileStore`).
- **Does:** Documents the internal C++ classes, method signatures, and state machine transitions in the bridge.
- **Why:** Provides PC bridge developers with quick reference documentation for bridge internals.

#### `docs/ARCHITECTURE.md`
- **Contains:** Architectural overview, data flow diagrams, simulator interfaces, multi-COM fanout designs, and subscription filtering mechanisms.
- **Does:** Explains how DCS World, the PC bridge, RS-485 master boards, and slave panels interact.
- **Why:** Gives contributors a clear conceptual model of system data flow and subsystem boundaries.

#### `docs/DCS_DATA_PROVENANCE.md`
- **Contains:** Generated register recording DCS device numbers, argument numbers, and command IDs for every control.
- **Does:** Documents where DCS cockpit bindings originated and notes unverified controls.
- **Why:** Ensures traceability of cockpit data bindings against the DCS F/A-18C module.

#### `docs/DEVELOPER_GUIDE.md`
- **Contains:** Current development status, verified workflows, architectural roadmap, and prioritized future-work backlog.
- **Does:** Acts as the engineering status ledger and roadmap for the project.
- **Why:** Prevents duplicate effort and coordinates feature progress across development sessions.

#### `docs/F18C_CONTROL_REFERENCE.md`
- **Contains:** Generated per-panel reference of every F/A-18C cockpit control, identifier, Kind (Switch, Selector, Button, etc.), and valid positions.
- **Does:** Tells cockpit builders the exact C++ identifiers to use in panel sketches (e.g., `MasterArm::MasterArm`).
- **Why:** Provides an easy lookup table for wiring hardware to cockpit controls.

#### `docs/F18C_EXPORT_INVENTORY.md`
- **Contains:** Full listing of all 505 documented DCS-BIOS controls grouped into 74 equipment categories.
- **Does:** Records memory addresses, bitmasks, bit-shifts, and interaction types for legacy DCS-BIOS compatibility.
- **Why:** Serves as a reference when mapping legacy DCS-BIOS panels to Hornet Link.

#### `docs/FIRMWARE_GUIDE.md`
- **Contains:** Hardware selection guide, wiring schematics, library installation instructions, and board comparison table.
- **Does:** Guides builders through flashing Arduino Mega 2560, Pro Micro, ESP32, and Arduino Giga boards.
- **Why:** Helps hardware makers assemble and flash working physical panels.

#### `docs/FIRST_PANEL.md`
- **Contains:** Step-by-step tutorial taking a beginner from a bare Arduino board to a working switch and LED in DCS.
- **Does:** Explains wiring, sketch writing with `Hornet.h`, and serial monitor debugging.
- **Why:** Offers the lowest-friction entry point for users building their first cockpit panel.

#### `docs/PROTOCOL_REFERENCE.md`
- **Contains:** Full wire-level specification for Protocol v1 (DCS-BIOS frames, handshake ping/pong, mode push/ack, import lines, and full RS-485 sub-bus spec).
- **Does:** Defines byte structures, framing, CRC-16 algorithms, timing, and error recovery for Protocol v1.
- **Why:** Authoritative specification for Protocol v1 communication.

#### `docs/PROTOCOL_V2.md`
- **Contains:** Complete specification for Protocol v2 (Hornet-native COBS framing, polled RS-485 bus, typed controls, state broadcast, input forwarding).
- **Does:** Specifies bus arbitration, collision-free polling cycles, sync reports, and operating modes.
- **Why:** Authoritative specification for Protocol v2 communication.

#### `docs/PROTOCOL_V2_MESSAGES.md`
- **Contains:** Generated message tables, message IDs, payload formats, address allocations, and enum values for Protocol v2.
- **Does:** Provides a quick tabular reference of all wire messages in Protocol v2.
- **Why:** Enables rapid lookup of message structures during protocol debugging.

#### `docs/RELEASE_PROCESS.md`
- **Contains:** Release tagging procedures, GitHub Actions packaging details, and release artifact verification steps.
- **Does:** Documents how to cut official version releases and verify published binaries.
- **Why:** Ensures releases are produced reliably and consistently.

---

### Arduino Firmware Library (`libraries/HornetLink/`)

#### `libraries/HornetLink/library.properties`
- **Contains:** Arduino library metadata (name, version, author, description, supported architectures).
- **Does:** Allows the Arduino IDE and `arduino-cli` to index, include, and compile the library.
- **Why:** Standard Arduino package definition file.

#### `libraries/HornetLink/src/Hornet.h`
- **Contains:** Top-level convenience include header for Protocol v2.
- **Does:** Imports `HornetTypes.h`, `HornetCatalog.h`, `HornetIO.h`, `HornetFilters.h`, `HornetElements.h`, `HornetPanel.h`, and `generated/HornetF18C.h`.
- **Why:** Allows users to write `#include <Hornet.h>` and have access to the complete Protocol v2 panel API.

#### `libraries/HornetLink/src/HornetCatalog.h`
- **Contains:** In-memory control lookup tables and lookup functions (`findControlById`, `findControlByName`).
- **Does:** Resolves control IDs to control names and metadata on nodes with sufficient flash memory.
- **Why:** Enables runtime introspection and ASCII debug mode on panel boards.

#### `libraries/HornetLink/src/HornetElements.h`
- **Contains:** High-level cockpit element classes (`Switch`, `Selector`, `Button`, `Pot`, `Encoder`, `Lamp`, `Gauge`, `TextDisplay`).
- **Does:** Handles hardware contact debouncing, edge detection, position tracking, and value synchronization.
- **Why:** Shields panel sketch authors from low-level digital/analog pin polling and debouncing logic.

#### `libraries/HornetLink/src/HornetFilters.h`
- **Contains:** Signal conditioning algorithms: `Debouncer`, `PotFilter` (EMA + hysteresis), `QuadratureDecoder`, and `CurvePoint` calibration.
- **Does:** Filters noisy physical switch contacts and unstable analog potentiometer readings.
- **Why:** Ensures rock-solid physical input without jitter or spurious state changes.

#### `libraries/HornetLink/src/HornetIO.h`
- **Contains:** Hardware abstraction layer for digital and analog I/O (`DirectPins`, `DirectOutputs`, `Hc165`, `Hc595`, `Cd4067`, `Matrix`).
- **Does:** Unifies direct Arduino pins, 74HC165 shift registers, 74HC595 output registers, CD4067 analog multiplexers, and diode matrices behind common interfaces.
- **Why:** Allows cockpit elements to bind seamlessly to multiplexers and shift registers without altering sketch code.

#### `libraries/HornetLink/src/HornetLink.h`
- **Contains:** Top-level convenience include header for Protocol v1.
- **Does:** Imports `HornetLinkBase.h`, `HornetLinkSlave.h`, `HornetLinkMaster.h`, `HornetLinkCompatDcsBios.h`, `HornetLinkImport.h`, and `HornetLinkMode.h`.
- **Why:** Maintains backward compatibility for legacy DCS-BIOS based panels.

#### `libraries/HornetLink/src/HornetLinkBase.h`
- **Contains:** Protocol v1 frame definitions, header structs, CRC-16 computation, and `hl_subscription_t`.
- **Does:** Provides core types and CRC validation for Protocol v1 serial packets.
- **Why:** Shared protocol constants across v1 master and slave implementations.

#### `libraries/HornetLink/src/HornetLinkCompatDcsBios.h`
- **Contains:** Protocol parser compatibility class `DcsBios::ProtocolParser`.
- **Does:** Implements the DCS-BIOS byte-stream state machine (sync word detection, address parsing, payload dispatch).
- **Why:** Enables legacy sketches to parse raw DCS-BIOS streams without external libraries.

#### `libraries/HornetLink/src/HornetLinkImport.h`
- **Contains:** Helper class `HornetLinkImport` for formatting and sending ASCII import commands (`SET <CONTROL> <VAL>\n`).
- **Does:** Formats cockpit switch and button presses into DCS import lines.
- **Why:** Provides input capability for Protocol v1 slave devices.

#### `libraries/HornetLink/src/HornetLinkMaster.h`
- **Contains:** `HornetLinkMaster` class managing the primary USB link and downstream RS-485 slave bus.
- **Does:** Discovers slave panels, forwards filtered delta frames, collects import lines, and propagates mode changes.
- **Why:** Core controller engine for Protocol v1 RS-485 bus master hardware (e.g. Mega 2560).

#### `libraries/HornetLink/src/HornetLinkMode.h`
- **Contains:** Definitions for operating modes (`kModeSim`, `kModePreflight`, `kModeMaintenance`) and mode callback handlers.
- **Does:** Manages mode transitions (e.g., triggering lamp tests during Preflight mode).
- **Why:** Provides system-wide mode coordination between the PC bridge and panels.

#### `libraries/HornetLink/src/HornetLinkSlave.h`
- **Contains:** `HornetLinkSlave` class for panel boards attached to a Protocol v1 RS-485 sub-bus.
- **Does:** Listens for addressed packets, validates CRCs, parses DCS-BIOS delta data, and replies to keep-alives.
- **Why:** Core firmware engine for Protocol v1 slave panels (e.g. Pro Micro).

#### `libraries/HornetLink/src/HornetMcp23017.h`
- **Contains:** Driver for Microchip MCP23017 16-bit I2C I/O expander chips.
- **Does:** Configures pin directions, internal pull-ups, and reads/writes 16-bit port registers over I2C.
- **Why:** Supports high-density cockpit panels with up to 16 digital I/O pins per I2C chip.

#### `libraries/HornetLink/src/HornetPanel.h`
- **Contains:** Main Protocol v2 application classes: `Panel`, `BusMaster`, and `StreamPort`.
- **Does:** Manages panel lifecycle (`beginDebug`, `beginUsb`, `beginRs485`), element registration, polling, sync reports, and bus arbitration.
- **Why:** The central high-level API used by panel sketches in Protocol v2.

#### `libraries/HornetLink/src/HornetTypes.h`
- **Contains:** Core enums (`Kind::Switch`, `Kind::Button`, etc.), `Control<K, N>` descriptor templates, and `ControlInfo`.
- **Does:** Provides strongly-typed descriptors that bind physical controls to catalogue definitions.
- **Why:** Enables compile-time checking of cockpit control types, preventing mismatched wiring in sketches.

#### `libraries/HornetLink/src/generated/HornetF18C.h`
- **Contains:** Generated typed control definitions and Flash string tables for every F/A-18C cockpit control.
- **Does:** Exposes control names like `Hornet::MasterArm::MasterArm` and `Hornet::Ufc::Key1` as typed objects.
- **Why:** Eliminates magic numbers and address mapping errors in sketch code.

#### `libraries/HornetLink/src/protocol/HnFrame.h`
- **Contains:** Protocol v2 COBS encoder/decoder (`CobsWriter`, `FrameDecoder`), Kermit CRC-16, and `DuplicateFilter`.
- **Does:** Frames raw byte streams into verified, delimited packets over UART or USB.
- **Why:** Guarantees packet integrity and framing resynchronization over noisy physical lines.

#### `libraries/HornetLink/src/protocol/HnMaster.h`
- **Contains:** Protocol v2 `HnMaster::BusMaster` engine.
- **Does:** Manages polling cycles, discovers slaves, dispatches simulator state, and forwards slave inputs to the bridge.
- **Why:** Implements collision-free bus arbitration on half-duplex RS-485 buses.

#### `libraries/HornetLink/src/protocol/HnNode.h`
- **Contains:** Protocol v2 `HnNode::Node` engine, `Port` stream interface, and `NodeHandler` callbacks.
- **Does:** Processes incoming frames, answers polls, transmits inputs, and formats diagnostic counters.
- **Why:** Provides the low-level protocol state machine running on every panel board.

#### `libraries/HornetLink/src/protocol/HnRecords.h`
- **Contains:** Binary record encoders and decoders (`StateWriter`, `StateReader`, `InputReader`, `SyncReader`, `Hello`, `Diag`).
- **Does:** Serializes and deserializes structured protocol payloads.
- **Why:** Ensures compact, efficient binary representations for high throughput over microcontrollers.

#### `libraries/HornetLink/src/protocol/HnSpec.h`
- **Contains:** Generated constants for Protocol v2 message types, address definitions, roles, actions, and limits.
- **Does:** Provides common protocol constants for both firmware and PC bridge code.
- **Why:** Guarantees wire-protocol compatibility across the entire stack.

---

### Example Firmware Sketches (`sketches/`)

#### `sketches/sketch_mega2560_master/sketch_mega2560_master.ino`
- **Contains:** Protocol v1 RS-485 bus master sketch for Arduino Mega 2560.
- **Does:** Bridges primary USB-CDC to downstream MAX485 transceiver on Serial1.
- **Why:** Provides a production-ready v1 master firmware example.

#### `sketches/sketch_pro_micro_slave/sketch_pro_micro_slave.ino`
- **Contains:** Protocol v1 RS-485 slave panel sketch for Arduino Pro Micro (ATmega32U4).
- **Does:** Receives DCS-BIOS updates over RS-485, drives LEDs, and sends switch input lines.
- **Why:** Provides a production-ready v1 slave panel firmware example.

#### `sketches/sketch_esp32_master/sketch_esp32_master.ino`
- **Contains:** Protocol v1 RS-485 bus master sketch for ESP32 boards.
- **Does:** Bridges USB serial to hardware UART with high baud rates on ESP32 hardware.
- **Why:** Provides a high-speed v1 master option with modern 32-bit hardware.

#### `sketches/v2_first_panel/v2_first_panel.ino`
- **Contains:** Minimal Protocol v2 panel example with one switch and one lamp.
- **Does:** Implements the introductory panel described in `docs/FIRST_PANEL.md` running in plain-text debug mode.
- **Why:** Offers the simplest code template for beginners learning Protocol v2.

#### `sketches/v2_master_arm_panel/v2_master_arm_panel.ino`
- **Contains:** Complete Protocol v2 Master Arm panel sketch for Pro Micro.
- **Does:** Implements Master Arm switch, A/A and A/G mode buttons, Ready light, and Disarm indicator.
- **Why:** Demonstrates a realistic combat panel implementation with mixed inputs and outputs.

#### `sketches/v2_caution_panel/v2_caution_panel.ino`
- **Contains:** Protocol v2 Caution Lights panel sketch driving multiple output indicators.
- **Does:** Subscribes to caution lamp states and drives status LEDs via 74HC595 shift registers.
- **Why:** Demonstrates dense output-only annunciator panels.

#### `sketches/v2_ufc_panel/v2_ufc_panel.ino`
- **Contains:** Protocol v2 Up-Front Controller (UFC) panel sketch for Mega 2560, ESP32, or Giga R1.
- **Does:** Reads numeric keypads, function buttons, rotary encoders, and displays scratchpad text.
- **Why:** Demonstrates a complex, high-density avionics interface with alphanumeric displays.

#### `sketches/v2_bus_master/v2_bus_master.ino`
- **Contains:** Protocol v2 RS-485 bus master firmware for Mega 2560, ESP32, or Arduino Giga R1.
- **Does:** Coordinates bus arbitration, slave polling, and packet routing between the PC bridge and slave panels.
- **Why:** Turnkey bus controller firmware for multi-panel cockpits using Protocol v2.

---

### Windows Serial Bridge (`Programs/dcsbios-serial-bridge/`)

#### `CMakeLists.txt`
- **Contains:** CMake build configuration for the Windows bridge executable (`hornet-link.exe`).
- **Does:** Configures compilers, defines Windows macros (`UNICODE`, `WIN32_LEAN_AND_MEAN`), links Win32 libraries (`ws2_32`, `shlwapi`, `comdlg32`), and sets up CPack installers.
- **Why:** Provides cross-platform CMake build generation for MSVC and MinGW.

#### `README.md`
- **Contains:** Bridge documentation, GUI feature list, CLI startup flags, and build instructions.
- **Does:** Introduces users and developers to the Windows bridge application.
- **Why:** Local documentation for the bridge component.

#### `build-package.cmd`
- **Contains:** Windows batch script to build Release binaries and bundle runtime files into a distribution zip.
- **Does:** Automates clean Release builds and packaging for end-user distribution.
- **Why:** Simplifies creating standalone release archives without manual file copying.

#### `known-bugs.md`
- **Contains:** Tracking log for open and resolved bridge bugs (e.g. status text flicker).
- **Does:** Records cosmetic and edge-case issues under investigation.
- **Why:** Keeps bug notes organized and accessible.

#### `make-installer.cmd`
- **Contains:** Windows batch script driving CPack to generate ZIP and NSIS installer packages.
- **Does:** Automates installer generation for release workflows.
- **Why:** Used by CI to produce packaged installers.

#### `quick-start.cmd`
- **Contains:** Helper script to quickly build and launch the bridge in Debug or Release mode.
- **Does:** Streamlines local iteration during development on Windows.
- **Why:** Provides a one-click build-and-run workflow.

#### `setup-imgui.cmd`
- **Contains:** Script to fetch and configure Dear ImGui dependencies for alternate GUI backends.
- **Does:** Sets up third-party GUI files if building the experimental ImGui interface.
- **Why:** Isolates optional external GUI dependencies.

#### `test-virtual-com.cmd`
- **Contains:** Test script setting up paired virtual COM ports for local loopback testing.
- **Does:** Configures com0com pairs to verify bridge serial behavior without physical hardware.
- **Why:** Enables automated and manual bridge verification on headless development systems.

#### `serial-monitor.py`
- **Contains:** Python script monitoring and echoing raw traffic on a serial port.
- **Does:** Reads serial packets and prints decoded byte streams to the terminal.
- **Why:** Diagnostic tool for validating serial communication independently of the GUI.

#### `lua/HornetLinkExport.lua`
- **Contains:** Lua export script forwarding DCS-BIOS telemetry over UDP.
- **Does:** Hooks DCS World telemetry loops and broadcasts cockpit data over local UDP sockets.
- **Why:** Provides the telemetry feed from DCS World into the bridge for Protocol v1.

#### `lua/HornetLinkNative.lua`
- **Contains:** Generated Lua exporter for Protocol v2 (Hornet-native).
- **Does:** Gathers cockpit argument and indication states directly from DCS World without DCS-BIOS.
- **Why:** Native, zero-dependency DCS telemetry export for Protocol v2.

#### `templates/panels.json`
- **Contains:** Reference subscription templates for common cockpit panels.
- **Does:** Defines which addresses and controls belong to specific physical panels.
- **Why:** Seed configuration for the bridge's profile management.

#### `src/BiosProtocol.hpp`
- **Contains:** Protocol v1 DCS-BIOS parser (`ExportParser`), state map (`BiosStateMap`), and delta frame builder.
- **Does:** Parses raw DCS-BIOS byte streams, tracks changed addresses, and encodes subscription-filtered delta frames.
- **Why:** Core engine for Protocol v1 telemetry processing.

#### `src/ControlDatabase.hpp`
- **Contains:** In-memory control definition database.
- **Does:** Maps DCS-BIOS addresses and control names to metadata and categories.
- **Why:** Provides human-readable control lookups in bridge logs and UI.

#### `src/DcsBiosSource.hpp`
- **Contains:** `DcsBiosSource` implementation of `ISimSource`.
- **Does:** Connects to UDP multicast (`239.255.50.10:5010`) or TCP sockets to receive DCS-BIOS streams.
- **Why:** Standard telemetry receiver for users running DCS-BIOS.

#### `src/DcsDirectSource.hpp`
- **Contains:** `DcsDirectSource` implementation of `ISimSource`.
- **Does:** Listens on UDP port 42002 for direct frames from `HornetLinkExport.lua`.
- **Why:** Low-latency alternative telemetry receiver bypassing multicast.

#### `src/DeviceRegistry.hpp`
- **Contains:** Device connection manager, handshake parser (`HandshakeParser`), and device descriptors.
- **Does:** Manages COM port lifecycles, negotiates ping/pong handshakes, and tracks device subscription filters.
- **Why:** Central registry of all active physical panel connections.

#### `src/HornetNative.hpp`
- **Contains:** Protocol v2 native types: `CatalogState`, `LinkSession`, `SyncTracker`, and telemetry parsers.
- **Does:** Manages state tracking, initial sync reconciliation, and named control updates for Protocol v2.
- **Why:** Core bridge logic for Protocol v2 simulator communication.

#### `src/HornetNativeLink.hpp`
- **Contains:** Protocol v2 framing and serial session logic on the bridge side.
- **Does:** Encodes COBS frames, tracks sequence numbers, handles ACKs/NACKs, and routes inputs.
- **Why:** Bridges PC bridge data structures onto the serial wire for Protocol v2.

#### `src/HornetNativeSource.hpp`
- **Contains:** `HornetNativeSource` implementation of `ISimSource`.
- **Does:** Listens on UDP port 42003 for native datagrams from `HornetLinkNative.lua` and sends inputs to 42004.
- **Why:** Native DCS World telemetry receiver for Protocol v2.

#### `src/LogPost.hpp`
- **Contains:** Bounded thread-safe logging queue and channel filtering (`kLogSerial`, `kLogDcs`, `kLogSystem`).
- **Does:** Formats timestamped log messages and dispatches them to UI windows and disk files.
- **Why:** Provides responsive logging without stalling real-time communication threads.

#### `src/MsfsSource.hpp`
- **Contains:** Placeholder `MsfsSource` implementation of `ISimSource`.
- **Does:** Provides an interface stub for future Microsoft Flight Simulator SimConnect integration.
- **Why:** Architecture placeholder for future multi-simulator support.

#### `src/ProfileStore.hpp`
- **Contains:** Persistent configuration store (`ProfileStore`) with atomic file writes.
- **Does:** Loads, caches, and saves per-device panel profiles and subscription overrides to JSON.
- **Why:** Allows physical panels to retain their configuration across bridge restarts.

#### `src/RS485ProtocolSpec.hpp`
- **Contains:** Protocol v1 wire constants, frame structures, and Kermit CRC-16 implementation.
- **Does:** Encodes and validates RS-485 sub-bus packets on the PC bridge side.
- **Why:** Shared wire-level protocol definitions for Protocol v1.

#### `src/ReplayFileSource.hpp`
- **Contains:** `ReplayFileSource` implementation of `ISimSource`.
- **Does:** Reads pre-recorded binary telemetry capture files and replays them with calibrated timing.
- **Why:** Enables offline development and repeatable testing without running DCS World.

#### `src/SimSource.hpp`
- **Contains:** Abstract base interface `ISimSource` and source state enumerations.
- **Does:** Decouples simulator telemetry sources from bridge core processing.
- **Why:** Enables hot-swapping between DCS-BIOS, native UDP, replay files, and future simulators.

#### `src/TextConv.hpp`
- **Contains:** UTF-8 / UTF-16 conversion helpers (`ToWide`, `ToUtf8`).
- **Does:** Converts character encodings between Win32 Unicode APIs and standard C++ strings.
- **Why:** Ensures clean Unicode handling across Windows API boundaries.

#### `src/device_profiles.json`
- **Contains:** Shipped default device configuration profiles.
- **Does:** Provides initial subscription configurations for known panel hardware.
- **Why:** Default configuration shipped alongside the application.

#### `src/main.cpp`
- **Contains:** Windows GUI application entry point, message pump, window layout, and event handlers.
- **Does:** Initializes the Win32 window, renders controls, connects serial ports, and runs the bridge loop.
- **Why:** Primary user-facing desktop application.

#### `src/main_imgui.cpp`
- **Contains:** Alternative Dear ImGui frontend implementation.
- **Does:** Renders an experimental hardware-accelerated GUI interface.
- **Why:** Serves as a prototype for modern, cross-platform UI rendering.

#### `src/version.rc.in`
- **Contains:** Windows resource script template for version information and file metadata.
- **Does:** Injects product name, version numbers, and copyright into the compiled `.exe`.
- **Why:** Standard Windows binary metadata integration.

---

### Test Suite & Quality Verification (`tests/`)

#### `tests/CMakeLists.txt`
- **Contains:** CMake configuration for building unit test targets (`hornet-link-tests`).
- **Does:** Configures compilers, enables address/undefined-behavior sanitizers (`-DHORNET_LINK_SANITIZE=ON`), and registers ctest targets.
- **Why:** Provides portable, cross-platform test builds for Linux, macOS, and Windows.

#### `tests/test-plan.yaml`
- **Contains:** Structured test plan cataloging unit, integration, and manual hardware test cases.
- **Does:** Tracks coverage requirements across protocol parsing, handshaking, delta frames, and bus arbitration.
- **Why:** Serves as a master checklist for test verification.

#### `tests/test_main.cpp`
- **Contains:** Main entry point and test runner for C++ test execution.
- **Does:** Initializes test fixtures, runs registered test suites, and reports pass/fail tallies.
- **Why:** Lightweight test runner with zero external dependencies.

#### `tests/test_framework.hpp`
- **Contains:** Header-only unit testing macros (`TEST_CASE`, `ASSERT_EQ`, `ASSERT_TRUE`, `ASSERT_FALSE`).
- **Does:** Provides an assertion framework similar to Catch2 or GoogleTest without external package overhead.
- **Why:** Enables fast, dependency-free test compilation across all supported compilers.

#### `tests/test_core.cpp`
- **Contains:** Unit tests for Protocol v1 core bridge logic (DCS-BIOS parser, delta frame generation, handshake decoder).
- **Does:** Verifies frame synchronization, address merging, subscription filtering, and CRC validation.
- **Why:** Prevents regressions in Protocol v1 bridge processing.

#### `tests/test_hornet_v2.cpp`
- **Contains:** Unit tests for Protocol v2 logic (COBS framing, simulated RS-485 bus master and slaves, state broadcasting).
- **Does:** Tests bus arbitration, duplicate rejection, drop/rejoin cycles, and corruption recovery.
- **Why:** Validates Protocol v2 behavior against simulated bus topologies.

#### `tests/test_profile_store.cpp`
- **Contains:** Unit tests for `ProfileStore` JSON parsing, override persistence, and atomic file saving.
- **Does:** Verifies that device subscriptions and configuration settings save and reload correctly.
- **Why:** Ensures reliability of device profile storage.

#### `tests/arduino_stub/Arduino.h`
- **Contains:** Mock Arduino runtime environment for host C++ compilation on Linux and Windows.
- **Does:** Emulates `millis()`, `micros()`, `pinMode()`, `digitalRead()`, `digitalWrite()`, `analogRead()`, and `Stream`.
- **Why:** Allows Arduino library firmware code to be compiled and tested natively under desktop C++ test runners.

#### `tests/fuzz/CMakeLists.txt`
- **Contains:** CMake configuration for building LLVM `libFuzzer` targets.
- **Does:** Configures fuzz testing binaries with sanitizer coverage instrumentation.
- **Why:** Enables automated continuous fuzz testing in CI.

#### `tests/fuzz/fuzz_*.cpp` (8 files)
- **Files:** `fuzz_export_parser.cpp`, `fuzz_handshake.cpp`, `fuzz_hn_frame.cpp`, `fuzz_hn_link.cpp`, `fuzz_import_line.cpp`, `fuzz_native_datagram.cpp`, `fuzz_profile_store.cpp`, `fuzz_rs485_frame.cpp`.
- **Contains:** libFuzzer target functions feeding mutated byte streams into parsers and decoders.
- **Does:** Stresses decoders to detect buffer overflows, unhandled edge cases, memory leaks, and crashes.
- **Why:** Guarantees robustness against malformed or malicious network/serial data.

---

## 3. Arduino Library Function Catalog & Reference

This section catalogs every public class, function, method, and constructor across `libraries/HornetLink/` for both Protocol v2 and Protocol v1.

---

### Protocol v2 High-Level Panel API (`HornetPanel.h`)

#### Class `Hornet::Panel`
The primary application coordinator for a physical cockpit panel.

```cpp
// Constructor
Panel(const char* name = "PANEL");
```
- **Use:** Initializes a panel with a human-readable identifier (up to 16 characters). This name is reported to the PC bridge during bus discovery.

```cpp
// Serial Debug Lifecycle
void beginDebug(Stream& serial = Serial);
```
- **Use:** Puts the panel in plain-text interactive Serial Monitor mode at 115,200 baud. Switch flips print `CONTROL=POSITION`; typing commands toggles lamps and controls. Ideal for bench testing without DCS.

```cpp
// USB Lifecycle
void beginUsb(Stream& serial = Serial);
```
- **Use:** Puts the panel into direct USB-CDC Protocol v2 binary mode, connecting directly to `hornet-link.exe` over a virtual COM port.

```cpp
// RS-485 Sub-Bus Lifecycle
void beginRs485(Stream& serial, uint8_t address, int dirPin = -1);
```
- **Use:** Configures the panel as a slave on an RS-485 bus.
  - `serial`: Hardware serial port connected to the RS-485 transceiver.
  - `address`: Fixed bus address (`0x01` through `0xEF`).
  - `dirPin`: GPIO pin controlling the transceiver `/RE` and `DE` lines (`-1` if auto-direction hardware is used).

```cpp
// Main Loop Execution
void update();
```
- **Use:** Must be called on every iteration of `void loop()`. Polls registered input elements, reads incoming frames, processes commands, and transmits queued events.

```cpp
// Registration (automatic when elements take &panel, or explicit)
void addElement(Element* elem);
```
- **Use:** Registers an input or output element with the panel lifecycle.

```cpp
// Operating Mode Callback
void onModeChange(void (*cb)(uint8_t mode));
```
- **Use:** Registers a callback that fires whenever the bridge changes the operating mode (`MODE_SIM`, `MODE_LAMP_TEST`, `MODE_MAINTENANCE`, `MODE_WIRING_TEST`).

---

#### Class `Hornet::BusMaster`
Coordinates a half-duplex RS-485 bus of downstream panel boards and bridges them to the PC.

```cpp
// Constructor
BusMaster();
```
- **Use:** Instantiates the bus arbiter.

```cpp
// Lifecycle
void begin(Stream& pcSerial, Stream& busSerial, int dirPin = -1);
```
- **Use:** Initializes master communication:
  - `pcSerial`: Primary USB-CDC link to `hornet-link.exe` (e.g. `Serial`).
  - `busSerial`: UART driving the RS-485 bus (e.g. `Serial1`).
  - `dirPin`: GPIO driving transceiver direction (`/RE` + `DE`).

```cpp
// Loop Execution
void update();
```
- **Use:** Executes bus arbitration. Sequentially polls known slaves, handles discovery windows for newly connected panels, and routes datagrams between USB and RS-485.

---

### Cockpit Element Classes (`HornetElements.h`)

All elements inherit from base `Element` and register themselves with the active `Panel`.

#### Class `Hornet::Switch`
Represents a multi-position toggle or rocker switch (2 or 3 positions).

```cpp
// Constructors
template <Kind K, uint8_t N, typename... In>
Switch(const Control<K, N>& control, In... contacts);
```
- **Parameters:**
  - `control`: Strongly-typed control token from `Hornet::` catalogue (e.g. `MasterArm::MasterArm`).
  - `contacts`: Digital inputs corresponding to each switch position. For a standard 2-position switch, a single pin is passed; the library uses an internal pull-up where LOW = position 1 (ON) and HIGH = position 0 (OFF). For 3-position ON-OFF-ON switches, two pins are passed along with `Hornet::none` for the floating center position.
- **Use:** Binds physical toggle switches to cockpit switch controls.

```cpp
Switch& holdCoil(DigitalOut coilPin, uint8_t heldPosition);
```
- **Use:** Configures a magnetic hold coil (e.g. FCS BIT switch or Anti-Skid switch). Energizes `coilPin` when DCS reports the switch is held; releases the coil when DCS trips it.

```cpp
Switch& invert();
```
- **Use:** Reverses physical contact polarity if the switch was wired backwards.

---

#### Class `Hornet::Selector`
Represents a multi-position rotary selector switch.

```cpp
template <Kind K, uint8_t N, typename... In>
Selector(const Control<K, N>& control, In... contacts);
```
- **Parameters:**
  - `control`: Strongly-typed rotary selector descriptor (e.g. `MasterArm::MasterArm` or sensor panel selectors).
  - `contacts`: N digital input contacts (direct pins or shift register inputs), one per switch detent.
- **Use:** Automatically decodes 3 to 12 position rotary switches and sends position indices to DCS.

---

#### Class `Hornet::Button`
Represents a momentary pushbutton.

```cpp
template <Kind K, uint8_t N>
Button(const Control<K, N>& control, DigitalIn input);
```
- **Parameters:**
  - `control`: Pushbutton descriptor (e.g. `Ufc::Key1`).
  - `input`: Digital input pin or multiplexer channel.
- **Use:** Debounces button presses, firing `PRESS` actions on down-strokes and `RELEASE` actions on release.

---

#### Class `Hornet::Pot`
Represents an analog potentiometer (e.g. volume knobs, panel lighting dimmers).

```cpp
template <Kind K, uint8_t N>
Pot(const Control<K, N>& control, AnalogIn pin);
```
- **Parameters:**
  - `control`: Analog axis/rotary descriptor.
  - `pin`: Analog input pin (e.g. `A0`) or CD4067 multiplexer channel.
- **Use:** Applies exponential smoothing and deadband filtering, sending position updates (0–65535) only when the knob is physically turned.

```cpp
Pot& invert();
```
- **Use:** Inverts the analog axis direction (maps 1023 -> 0).

```cpp
Pot& calibrate(uint16_t minVal, uint16_t maxVal);
```
- **Use:** Sets hardware end-stop limits to ensure full 0–100% travel range.

---

#### Class `Hornet::Encoder`
Represents a rotary encoder with quadrature outputs (A/B phases).

```cpp
template <Kind K, uint8_t N>
Encoder(const Control<K, N>& control, DigitalIn pinA, DigitalIn pinB, uint8_t pulsesPerDetent = 4);
```
- **Parameters:**
  - `control`: Rotary control descriptor.
  - `pinA`, `pinB`: Quadrature phase input pins.
  - `pulsesPerDetent`: Number of state transitions per click (typically 2 or 4).
- **Use:** Tracks incremental clockwise/counter-clockwise clicks and sends step adjustments to DCS.

---

#### Class `Hornet::Lamp`
Represents an annunciator or indicator LED.

```cpp
template <Kind K, uint8_t N>
Lamp(const Control<K, N>& control, DigitalOut pin);
```
- **Parameters:**
  - `control`: Indicator descriptor (e.g. `MasterArm::ReadyLt`).
  - `pin`: Output pin (direct GPIO or 74HC595 shift register output).
- **Use:** Listens for simulator state changes and drives the LED. Automatically illuminates during Lamp Test (`MODE_LAMP_TEST`).

```cpp
Lamp& invert();
```
- **Use:** Inverts output logic for active-low LED drivers.

---

#### Class `Hornet::Gauge`
Represents an analog voltmeter, servo, or PWM-driven instrument gauge.

```cpp
template <Kind K, uint8_t N>
Gauge(const Control<K, N>& control, void (*onUpdate)(uint16_t value));
```
- **Parameters:**
  - `control`: Gauge descriptor.
  - `onUpdate`: Callback invoked with current needle value (0–65535).
- **Use:** Drives instruments such as analog engine gauges or the mechanical clock.

---

#### Class `Hornet::TextDisplay`
Represents an alphanumeric string display (e.g. UFC scratchpad or comm displays).

```cpp
template <Kind K, uint8_t N>
TextDisplay(const Control<K, N>& control, void (*onUpdate)(const char* text));
```
- **Parameters:**
  - `control`: Text field descriptor (e.g. `Ufc::ScratchpadNumber`).
  - `onUpdate`: Callback invoked with updated null-terminated string.
- **Use:** Updates LCDs, OLEDs, or multi-segment character displays when DCS cockpit text changes.

---

### Hardware Input/Output Drivers (`HornetIO.h`, `HornetMcp23017.h`)

#### Input Wrappers
- `Hornet::input(uint8_t pin)`: Constructs a `DigitalIn` using the microcontroller's internal pull-up (`INPUT_PULLUP`).
- `Hornet::inputNoPullup(uint8_t pin)`: Constructs a `DigitalIn` with floating input (`INPUT`).
- `Hornet::output(uint8_t pin)`: Constructs a `DigitalOut` configured for push-pull output.
- `Hornet::analog(uint8_t pin)`: Constructs an `AnalogIn` reading ADC values (0–1023).

#### Shift Registers & Multiplexers
- `Hornet::Hc165<Chips>`: Cascaded 74HC165 8-bit parallel-in/serial-out shift registers.
  - `read()`: Latches parallel inputs and clocks in bits.
  - `in(uint8_t index)`: Returns a `DigitalIn` handle for channel `index`.
- `Hornet::Hc595<Chips>`: Cascaded 74HC595 8-bit serial-in/parallel-out shift registers.
  - `write()`: Transfers buffered output states to physical pins.
  - `out(uint8_t index)`: Returns a `DigitalOut` handle for channel `index`.
- `Hornet::Cd4067`: 16-channel analog/digital multiplexer using 4 address select pins.
  - `channel(uint8_t ch)`: Returns an `AnalogIn` or `DigitalIn` for selected channel.
- `Hornet::Matrix<Rows, Cols>`: Row-column keypad scanning matrix with diode isolation.
  - `button(uint8_t r, uint8_t c)`: Returns a `DigitalIn` handle for the intersecting button.
- `Hornet::Mcp23017`: 16-bit I2C GPIO expander.
  - `begin(uint8_t i2cAddr = 0x20)`: Configures I2C bus and chip registers.
  - `pinMode(uint8_t pin, uint8_t mode)`: Configures direction and pull-ups.
  - `digitalRead(uint8_t pin)`: Reads digital state.
  - `digitalWrite(uint8_t pin, uint8_t val)`: Sets output latch.

---

### Signal Conditioning & Math Filters (`HornetFilters.h`)

#### Class `Hornet::Debouncer`
```cpp
bool update(bool rawState, uint32_t nowMs);
```
- **Use:** Filters mechanical contact chatter using configurable debounce thresholds (default 15 ms).

#### Class `Hornet::PotFilter`
```cpp
uint16_t update(uint16_t rawAdc);
```
- **Use:** Filters potentiometer noise using an Exponential Moving Average (EMA) and a deadband window (default ±8 ADC units) to prevent continuous stream spam.

#### Class `Hornet::QuadratureDecoder`
```cpp
int8_t update(bool pinA, bool pinB);
```
- **Use:** Evaluates Gray code state transitions from optical or mechanical quadrature encoders, returning `+1` (clockwise), `-1` (counter-clockwise), or `0` (no change).

---

### Catalogue Types & Lookups (`HornetTypes.h`, `HornetCatalog.h`, `HornetF18C.h`)

#### Template `Hornet::Control<Kind K, uint8_t N>`
- Strongly-typed identifier binding:
  - `id`: Unique 16-bit control identifier from the catalogue.
  - `kind`: Control category (`Kind::Switch`, `Kind::Selector`, `Kind::Button`, `Kind::Pot`, `Kind::Encoder`, `Kind::Lamp`, `Kind::Gauge`, `Kind::Text`).
  - `positions`: Number of discrete switch positions or steps.

#### Functions in `HornetCatalog.h`
```cpp
const ControlInfo* findControlById(uint16_t id);
const ControlInfo* findControlByName(const char* name);
```
- **Use:** Resolves control names, identifiers, kinds, and positions at runtime.

---

### Protocol v2 Framing & Bus Engines (`protocol/Hn*.h`)

#### Functions in `HnFrame.h`
```cpp
size_t encodeFrame(uint8_t dst, uint8_t src, uint8_t seq, uint8_t type,
                   const uint8_t* payload, size_t payloadLen,
                   uint8_t* outBuf, size_t outMax);
```
- **Use:** Encloses payload in a 6-byte header, computes Kermit CRC-16, applies COBS byte stuffing, and appends a `0x00` frame delimiter.

```cpp
bool FrameDecoder::feed(uint8_t byte, Result& res);
```
- **Use:** Byte-by-byte streaming state machine decoding incoming COBS bytes and checking CRC-16.

```cpp
uint16_t computeCrc16(const uint8_t* data, size_t len);
```
- **Use:** Calculates CCITT-FALSE / Kermit CRC-16 across arbitrary buffers.

---

### Protocol v1 Legacy Sub-Bus & DCS-BIOS Bridge (`HornetLink*.h`)

#### Class `HornetLinkSlave` (`HornetLinkSlave.h`)
```cpp
HornetLinkSlave(uint8_t busAddress, int dirPin = -1);
void begin(HardwareSerial& serial, uint32_t baud = 250000);
void loop();
void setModeHandler(void (*handler)(uint8_t mode));
bool sendImport(const char* line);
```
- **Use:** Powers legacy DCS-BIOS slave boards on an RS-485 bus. Receives filtered delta frames, updates local outputs, and sends ASCII import lines to the master.

#### Class `HornetLinkMaster` (`HornetLinkMaster.h`)
```cpp
HornetLinkMaster(HardwareSerial& busSerial, int dirPin = -1);
void begin(Stream& pcStream, uint32_t busBaud = 250000);
void loop();
void registerSlave(uint8_t address, const char* name, const hl_subscription_t* subs, size_t subCount);
```
- **Use:** Runs on an Arduino Mega 2560 or ESP32 acting as a Protocol v1 RS-485 master. Bridges PC DCS-BIOS data to slave panels.

#### Class `DcsBios::ProtocolParser` (`HornetLinkCompatDcsBios.h`)
```cpp
void processChar(uint8_t c);
```
- **Use:** State machine detecting `0x55 0x55 0x55 0x55` sync words and dispatching 16-bit address write records to registered subscribers.

---

## 4. Agent & Contributor Document Navigation

Use this navigation table to locate detailed information across the repository:

| Objective | Document |
|---|---|
| **AI Agent Guidelines & Architectural Invariants** | [`AGENTS.md`](AGENTS.md) |
| **Human Contributor Workflow & Local Setup** | [`CONTRIBUTING.md`](CONTRIBUTING.md) |
| **Full File Inventory & Arduino Function Catalog** | [`REFERENCE.md`](REFERENCE.md) *(this document)* |
| **System Architecture & Data Flow** | [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) |
| **Protocol v1 Wire Specification (DCS-BIOS & RS-485)** | [`docs/PROTOCOL_REFERENCE.md`](docs/PROTOCOL_REFERENCE.md) |
| **Protocol v2 Specification (Hornet-Native COBS)** | [`docs/PROTOCOL_V2.md`](docs/PROTOCOL_V2.md) |
| **Protocol v2 Message Tables & IDs** | [`docs/PROTOCOL_V2_MESSAGES.md`](docs/PROTOCOL_V2_MESSAGES.md) |
| **F/A-18C Cockpit Control Identifiers** | [`docs/F18C_CONTROL_REFERENCE.md`](docs/F18C_CONTROL_REFERENCE.md) |
| **Step-by-Step First Panel Tutorial** | [`docs/FIRST_PANEL.md`](docs/FIRST_PANEL.md) |
| **Hardware Selection & Firmware Flashing** | [`docs/FIRMWARE_GUIDE.md`](docs/FIRMWARE_GUIDE.md) |
| **Engineering Status & Future Roadmap** | [`docs/DEVELOPER_GUIDE.md`](docs/DEVELOPER_GUIDE.md) |
| **Release Packaging & Tagging** | [`docs/RELEASE_PROCESS.md`](docs/RELEASE_PROCESS.md) |
