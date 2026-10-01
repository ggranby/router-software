# Contributing to Hornet Link

Thank you for your interest in contributing to Hornet Link! Contributions are accepted under the repository license (Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International, see [LICENSE](LICENSE)).

Before starting work:
- Review [REFERENCE.md](REFERENCE.md) for the master file-by-file inventory and Arduino library function catalog.
- If you are configuring or working with AI agents, see [AGENTS.md](AGENTS.md).
- Consult [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) for engineering status and the feature backlog.

---

## Contribution Workflow

1. **Fork & Branch:** Fork the repository and create a descriptive feature branch from `main`.
2. **Focused Changes:** Keep pull requests focused on a single feature, bug fix, or panel addition. Avoid combining refactoring or mass formatting with behavioral changes.
3. **Tests First:** Add or update automated tests in `tests/` for protocol, parser, or state changes. If your change requires physical hardware (e.g. specific Arduino boards or cockpit switches), describe your manual verification steps in the PR description.
4. **Code Generation:** If modifying cockpit controls or Protocol v2 definitions, edit `catalog/fa18c.json` or `catalog/protocol_v2.json`, then re-run `python3 Programs/tools/generate_hornet_catalog.py`. Do not manually edit generated files.
5. **Documentation:** Update relevant documents in `docs/` alongside code changes. Keep `REFERENCE.md` updated with new files or library functions.
6. **Changelog:** Add an entry under `[Unreleased]` in [CHANGELOG.md](CHANGELOG.md).
7. **CI Verification:** Ensure all GitHub Actions CI checks pass on your branch.

---

## Local Development & Testing

### 1. C++ Unit Tests (Linux / GCC with Sanitizers)

The test suite runs portably on Linux using a lightweight Arduino stub:

```bash
cmake -S tests -B build-tests -DCMAKE_BUILD_TYPE=Debug -DHORNET_LINK_SANITIZE=ON
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

### 2. C++ Unit Tests (Windows / MSVC)

```powershell
cmake -S tests -B tests/build -G "Visual Studio 17 2022" -A x64
cmake --build tests/build --config Release
ctest --test-dir tests/build -C Release --output-on-failure
```

### 3. Windows Bridge Build (Visual Studio)

```powershell
cd Programs/dcsbios-serial-bridge
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Or cross-compile on Linux with MinGW:

```bash
cd Programs/dcsbios-serial-bridge
x86_64-w64-mingw32-g++-posix -std=c++17 -municode -mwindows \
  -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX \
  -Wall -Wextra -Werror -Wno-unknown-pragmas \
  src/main.cpp -o hornet-link.exe -static \
  -lws2_32 -lshlwapi -lcomdlg32
```

### 4. Catalogue Consistency Check

Verify that generated files match the catalogue sources:

```bash
python3 Programs/tools/generate_hornet_catalog.py --check
```

### 5. Arduino Sketch Compilation

Using `arduino-cli`:

```bash
# Verify Protocol v2 panels
arduino-cli compile --libraries libraries --fqbn arduino:avr:leonardo sketches/v2_first_panel
arduino-cli compile --libraries libraries --fqbn arduino:avr:mega sketches/v2_bus_master
```

### 6. Lua Linter

```bash
cd Programs/dcsbios-serial-bridge/lua
luacheck .
```

---

## Code Style & Architectural Rules

- **Bridge (C++17):** 4-space indentation, no tabs. Format new or edited code in accordance with `.clang-format` and `.editorconfig`.
- **Firmware (C++11):** Arduino library headers in `libraries/HornetLink/` must stay compatible with C++11 and `avr-gcc 7.3`.
  - **Zero Dynamic Allocation:** Never use `malloc`, `free`, `new`, `delete`, or dynamic STL containers in firmware.
  - **No Virtual Destructors on Bases:** Base classes must use `protected: ~Base() = default;` to prevent dragging `operator delete` into AVR binaries.
  - **No CTAD:** Avoid C++17 Class Template Argument Deduction in library headers.
- **Defensive Parsing:** Treat all serial and network data as untrusted. Strictly bounds-check all incoming frames, lengths, and indices.
- **Clean Commits:** Do not commit build artifacts (`build/`, `build-tests/`, `tests/build/`, `.exe`), generated documentation (`docs/doxygen/`), or credentials/secrets.

---

## Reporting Issues & Security

- For bug reports and feature suggestions, open an issue on GitHub.
- For security vulnerabilities, follow the reporting process outlined in [SECURITY.md](SECURITY.md).
