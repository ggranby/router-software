# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the bridge follows
[Semantic Versioning](https://semver.org/). The bridge version is set in
`Programs/dcsbios-serial-bridge/CMakeLists.txt`; the Arduino library version is
in `libraries/HornetLink/library.properties`.

## [Unreleased]

### Added — Hornet-native protocol v2 (stage 1)
- F/A-18C control catalogue (`catalog/fa18c.json`) with stable IDs, readable
  names, types and position names. The generator
  (`Programs/tools/generate_hornet_catalog.py`) turns it into the Arduino
  header, the protocol constants, the native Lua exporter, the reference docs
  and a catalogue hash.
- Protocol v2: COBS framing with CRC-16, a polled RS-485 bus (no collisions),
  one STATE broadcast for all slaves, typed inputs with ACK/NACK reasons,
  duplicate detection and retry, SYNC, DIAG, CONFIG, fixed addresses with
  optional discovery. See `docs/PROTOCOL_V2.md`.
- Arduino API `Hornet.h`:
  - Named elements: Switch, Selector, Button, Pot, Encoder, Lamp, Gauge,
    TextDisplay. A wrong binding fails at compile time.
  - IO sources: pins, 74HC165/595, MCP23017, matrix, CD4067.
  - Built-in debounce, pot and encoder filters.
  - USB, RS-485 and ASCII debug transports.
  - Lamp test and wiring test.
  - Five example sketches (`sketches/v2_*`).
- `HornetLinkNative.lua` exporter. It doesn't need DCS-BIOS, works only in the
  F/A-18C, and can be pasted into Export.lua.
- Bridge-side v2 logic (`HornetNative.hpp`): exporter parser, input
  validation, DCS-is-truth sync tracker with an optional overlay, readable
  frame decoder, `LinkSession`. It is not yet wired into the bridge UI (stage 2).
- Tests: a v2 unit suite including a simulated 3-slave bus, three fuzz
  targets, a CI generator check, and v2 sketch builds for Leonardo, Mega,
  ESP32 and Arduino Giga.

### Added
- Singular master reference document (`REFERENCE.md`) containing a complete file-by-file directory breakdown, cleanup register, and full Arduino library function catalog.
- Agent operational reference (`AGENTS.md`) documenting architectural constraints, C++11 AVR portability rules, and CI check commands.

### Changed
- Condensed and consolidated `RS485_PROTOCOL.md` into `docs/PROTOCOL_REFERENCE.md` (Section 5), creating a single authoritative protocol reference.
- Enhanced `CONTRIBUTING.md` with comprehensive local setup instructions, cross-platform build commands, and coding standards.
- Updated `README.md` and `docs/DEVELOPER_GUIDE.md` references.

### Removed
- Removed redundant root and docs stubs: `DEVELOPMENT.md`, `docs/INCOMPLETE_ITEMS.md`, and `docs/RESUME_GUIDE.md`.
- Removed root `RS485_PROTOCOL.md` (consolidated into `docs/PROTOCOL_REFERENCE.md`).
- Removed obsolete socat batch scripts in `Programs/`: `connect-serial-port.cmd`, `ensure-socat.cmd`, `multicast-console.cmd`, and `multiple-com-ports.cmd`.
- The hand-written `lua/modules/FA-18C.lua` address map. It was inconsistent
  with DCS-BIOS and unused; `catalog/fa18c.json` replaces it.

### Fixed
- Export records near the end of the 64 KiB address space no longer write past
  the state map (network input).
- Wire-format log lines no longer crash on long control names.
- RS-485 master handshake: firmware now sends the slave list required by the
  protocol, and the bridge parses zero-subscription masters and slaves
  correctly (previously masters fell back to Legacy mode).
- Profile JSON parser no longer hangs on malformed input; profile saves are
  escaped and written atomically.
- Log messages are no longer leaked when the window is closing.
- Import lines that are overlong or contain invalid characters are rejected
  instead of being truncated and forwarded.
- UI strings containing non-ASCII characters are compiled as UTF-8 on MSVC.
- `library.properties` is now in the `key=value` format the Arduino tooling
  expects, with the correct repository URL.
  `HornetLink.h` moved into `src/` so the library installs and resolves as a
  standard Arduino 1.5 library; CI no longer injects include paths manually.

### Added
- Unit tests run in CI on Windows (MSVC) and Linux (GCC with ASan/UBSan) and
  gate the Windows build; the previous placeholder tests were replaced.
- libFuzzer targets for the export, handshake, import, RS-485 and profile
  parsers, with a short CI run.
- MinGW-w64 cross-compile check, luacheck for the Lua exporter, `.clang-tidy`.
- Version metadata: CMake project version drives the window title, the
  Windows VERSIONINFO resource and package names; releases check the tag.
- MSVC hardening flags (`/sdl`, `/guard:cf`, `/CETCOMPAT`).
- `SECURITY.md`, `CONTRIBUTING.md`, PR template, `.editorconfig`,
  `.clang-format`, Dependabot for GitHub Actions (actions pinned by SHA).
