# How to Continue This Conversation

This file gives you everything you need to resume work on Hornet Link in a new
GitHub Copilot chat session without losing significant context.

---

## Quick Status

| Area | Status |
|------|--------|
| PC bridge (`hornet-link.exe`) | ✅ Builds and runs |
| All HPP headers (sources, registry, protocol) | ✅ Complete |
| `main.cpp` UI + `BridgeController` | ✅ Complete |
| Arduino library (`libraries/HornetLink/`) | ✅ Complete |
| Firmware sketches (Mega, Pro Micro, ESP32) | ✅ Complete |
| Lua exporter + F/A-18C module | ✅ Complete |
| Panel templates (`templates/panels.json`) | ✅ Complete |
| Test plan (`tests/test-plan.yaml`) | ✅ Complete |
| CI workflow (`.github/workflows/ci.yml`) | ✅ Complete |
| `RS485_PROTOCOL.md` | ✅ Complete |
| `docs/ARCHITECTURE.md` | ✅ Complete |
| `docs/PROTOCOL_REFERENCE.md` | ✅ Complete |
| `docs/DEVELOPER_GUIDE.md` | ✅ Complete |
| `docs/FIRMWARE_GUIDE.md` | ✅ Complete |
| `docs/API_REFERENCE.md` | ✅ Complete |
| `docs/INCOMPLETE_ITEMS.md` | ✅ Complete |

All major deliverables are done.  See `docs/INCOMPLETE_ITEMS.md` for the ten
known gaps and how to address each one.

---

## Recommended Starter Prompt

Paste this at the beginning of a new chat session:

```
I'm continuing work on the Hornet Link project in the router-software workspace.
The project is a Win32 C++ bridge (hornet-link.exe) that feeds DCS-BIOS cockpit
data to Open-Hornet panels over USB serial and an RS-485 sub-bus.

The codebase is at:  c:\Users\ggran\Desktop\router-software

Key files to read first for context:
  docs/ARCHITECTURE.md           — overall design
  docs/API_REFERENCE.md          — all public C++ types
  docs/INCOMPLETE_ITEMS.md       — known gaps with fix guidance

The build command (from Programs\dcsbios-serial-bridge) is:
  cmake -S . -B build
  cmake --build build --config Release
The executable lands at build\Release\hornet-link.exe

I'd like to work on: [DESCRIBE WHAT YOU WANT TO DO]
```

Replace `[DESCRIBE WHAT YOU WANT TO DO]` with one of the tasks from
`docs/INCOMPLETE_ITEMS.md`, or describe a new feature.

---

## Key Technical Facts for the Agent

These are the most important facts to have in context. Include them or point to
`docs/API_REFERENCE.md` when prompting.

### Protocols
- **DCS-BIOS sync frame**: `0x55 0x55 0x55 0x55`; write record: `[addr_lo][addr_hi][len_lo][len_hi][data...]`
- **Handshake ping**: `AA DE AD 01`; pong: `AA DE AD 02 [flags][name_len][name][sub_count_lo][sub_count_hi][subs...]`; ack: `AA DE AD 03`
- **Mode push**: `AA DE AD 04 [mode]`; mode ack: `AA DE AD 05 [local_mode]`
- **RS-485 frame**: `[0xFE][dst][src][len_lo][len_hi][payload...][crc_lo][crc_hi]`
- **DcsDirectSource port**: UDP 42002

### Build
- Generator: Visual Studio 18 2022 (cached in `build/` — no `-G` flag needed)
- C++ standard: C++17
- Target name: `hornet-link`

### Directories
```
Programs/dcsbios-serial-bridge/src/    ← all C++ headers + main.cpp
libraries/HornetLink/src/              ← Arduino library headers
sketches/                              ← Arduino sketch folders
lua/                                   ← Lua export scripts
templates/                             ← panel JSON templates
docs/                                  ← documentation
```

---

## If You Want to Continue Without Pasting the Full Context

The agent can reconstruct context by reading the key docs:

1. `docs/ARCHITECTURE.md` — block diagram + file map
2. `docs/API_REFERENCE.md` — all public types
3. `docs/PROTOCOL_REFERENCE.md` — all wire protocols
4. `docs/INCOMPLETE_ITEMS.md` — what remains to do

Together these four files give a new agent session everything it needs to
understand the codebase and continue work.
