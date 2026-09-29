# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the bridge follows
[Semantic Versioning](https://semver.org/). The bridge version is set in
`Programs/dcsbios-serial-bridge/CMakeLists.txt`; the Arduino library version is
in `libraries/HornetLink/library.properties`.

## [Unreleased]

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
