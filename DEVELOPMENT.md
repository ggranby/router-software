# Hornet Link - Development Status & Roadmap

## Project Overview

**Hornet Link** is a Windows-based software bridge that connects DCS-BIOS export streams to custom Arduino-based cockpit hardware. It enables bidirectional communication between Digital Combat Simulator and physical control panels, switches, gauges, and instrument displays.

**Target Integration:** [OpenHornet](https://www.openhornet.com/) F/A-18C simulator projects and other DCS cockpit simulations.

**License:** CC BY-NC-SA 4.0 (see [LICENSE](LICENSE) for details)

---

## Current Development Status (v0.1 - WIP)

### Version: 0.1.0 (Pre-Release)
**Release Date:** April 2026  
**Stability:** Beta - Core functionality operational, feature set incomplete

### Feature Completeness Matrix

| Feature | Status | Notes |
|---------|--------|-------|
| **Core Networking** | ✅ Complete | UDP multicast & TCP modes fully functional |
| **Serial Port Management** | ✅ Complete | Multi-port fanout, auto-detect, 250k 8N1 |
| **GUI Framework** | ✅ Complete | Dark theme, resizable, responsive layout |
| **Device Handshake Protocol** | ✅ Complete | Probe/pong, subscription negotiation, RS-485 topology |
| **DCS-BIOS Integration** | ✅ Complete | State map parsing, JSON control reference loading |
| **Logging & Export** | ✅ Complete | Session logs, packet flow tracking, export to UTF-8 |
| **Dry-Run Mode** | ✅ Complete | Network-only testing without serial writes |
| **Startup Automation** | ✅ Complete | CLI flags for headless operation (`--autostart`, `--udp`, `--tcp`, `--ports=...`) |
| **Real-Time Metrics** | ✅ Complete | Frame counts, dispatch timing, serial write performance tracking |
| **Logging Channel Controls** | 🔄 In Progress | Selective log output filtering (planned for v0.2) |
| **Memory Management** | 🔄 In Progress | Bounded memory, optional full-capture mode (planned for v0.2) |
| **Board-to-Panel Mapping** | ❌ Planned | Multi-board discovery, device identity tracking, panel profile system (v0.3) |
| **Board Verification UI** | ❌ Planned | Heartbeat monitoring, handshake diagnostics, status indicators (v0.3) |
| **Configuration Persistence** | ⏳ Future | Save/restore COM port lists, panel maps, logging preferences (v0.3+) |
| **Hardware Compatibility Layer** | ⏳ Future | Support for non-Arduino serial devices, VID/PID matching (v0.4+) |

---

## Known Issues (v0.1)

### 1. UI Status Text Flicker
- **Severity:** Low (cosmetic)
- **Description:** Status line exhibits periodic visual flicker during operation
- **When:** Occurs during active logging with high message throughput (knob spam, multi-button presses)
- **Root Cause:** Frequent status updates without throttling; potential double-buffering issue
- **Workaround:** Reduce log volume by disabling non-critical channels (feature coming in v0.2)
- **Fix Timeline:** v0.2 (during logging refactor)

---

## Near-Term Streamlining Backlog (May 2026)

These are implementation-priority items focused on making setup and day-to-day use smoother,
without taking on MSFS integration work yet.

1. ✅ Runtime health and startup readiness reporting
2. End-to-end RS-485 discovery lifecycle (discover, keep-alive, offline transitions)
3. ✅ Replace heuristic frame forwarding with full frame-boundary parsing
4. ✅ One-command setup and run path for operators
5. First-run config generation with saved defaults
6. Profile UX improvements (auto-map known panel names, suggest defaults for unknown devices)
7. Operator-grade observability (rolling logs, key event markers)
8. Regression tests for discovery, framing, and profile fallback behavior

---

## Roadmap

### v0.2 (Q2 2026) - Logging & Memory Optimization
**Focus:** Reduce verbosity, improve long-session stability

**Planned Deliverables:**
1. **Logging Channel Controls**
   - Per-channel toggles: Control Changes, Raw Knobs/Dials, Raw Gauges, Transport Diagnostics
   - Default: Control Changes ON, Raw Knobs/Dials ON, Raw Gauges OFF, Diagnostics OFF
   - Runtime application with UI feedback
   - **Acceptance:** User can reduce high-volume channels without code changes

2. **Memory & Retention Policy**
   - Bounded ring buffer for UI text (configurable line limit)
   - Optional full-capture mode (stream-to-disk when ON)
   - Runtime memory usage visibility in status bar
   - **Acceptance:** No unbounded memory growth in multi-hour sessions

3. **Stress Testing & Performance Validation**
   - Automated test matrix: idle, switch spam, knob sweep, gauge-heavy scenarios
   - Metrics collection: queue depth, dropped lines, flush times, export accuracy
   - Regression testing framework
   - **Acceptance:** No UI freeze, stable queue behavior, export consistency

4. **Code Quality Pass**
   - Add comprehensive comments to all code paths
   - Document threading behavior, edge cases, non-obvious logic
   - Reorganize by function grouping and module boundaries
   - Remove dead code and debug-only paths
   - **Acceptance:** Clear readability for maintenance handoff

**Release Target:** June 2026

### v0.3 (Q3 2026) - Multi-Device Architecture & Board Management
**Focus:** Support complex multi-board cockpits with automatic device discovery

**Planned Deliverables:**
1. **Board Discovery Framework**
   - Enumerate connected serial devices
   - Collect stable identity hints (COM port, VID/PID, device name)
   - Track reconnects and port renumbering
   - Persistent device identity database

2. **Panel Mapping System**
   - GUI for creating/editing panel profiles
   - Assign boards to logical cockpit panels
   - Save/restore mappings automatically
   - Support for multi-board panels (e.g., left + right consoles)

3. **Board Verification & Diagnostics**
   - Per-device heartbeat/handshake monitoring
   - Visual status indicators (Connected / Degraded / No Data)
   - Test command generation and response validation
   - Roundtrip latency measurement

4. **Auto-Association Strategy (Phase 1)**
   - Exact match on persisted device identity
   - Fallback to user confirmation if ambiguous
   - Confidence scoring for identity matches
   - Never silently remap on low confidence

**Release Target:** September 2026

### v0.4 (Q4 2026) - Hardware Abstraction & Extended Compatibility
**Focus:** Support non-Arduino devices and diverse hardware platforms

**Planned Deliverables:**
1. **Hardware Profile System**
   - Device-specific configuration templates
   - Custom handshake protocols per hardware family
   - Baud rate / parity / stopbit flexibility
   - GPIO / CAN / SPI interface support (future)

2. **Device Drivers/Adapters**
   - Generic serial devices (configurable baud, framing)
   - Arduino compatibility layer (current default)
   - Teensy-specific optimizations
   - ESP32 WiFi bridge support (stretch goal)

3. **Enhanced VID/PID Matching**
   - Manufacturer/product matching for stable identification
   - Fallback strategies for unknown devices
   - Custom naming hints from device firmware

**Release Target:** December 2026

### v1.0 (2027) - Production Release
**Focus:** Stability, documentation, OpenHornet integration

**Planned Deliverables:**
1. Long-session stability validation (24+ hours)
2. Complete API documentation
3. OpenHornet software repository integration
4. Comprehensive user manual & tutorial videos
5. Community feedback integration
6. Installer and auto-updater

---

## Architecture Overview

### Threading Model
```
┌─────────────────────────────────────────────────────────┐
│                    Windows UI Thread                     │
│  (Message Loop, User Events, Menu/Dialog Handling)       │
└─────────────────────────────────────────────────────────┘
                           ↑ ↓
        ┌──────────────────────────────────────────────┐
        │         Network Thread (UDP/TCP)             │
        │  • Receives DCS-BIOS export stream           │
        │  • Parses protocol, extracts state changes   │
        │  • Queues dirty address words for dispatch   │
        └──────────────────────────────────────────────┘
                           ↑ ↓
        ┌──────────────────────────────────────────────┐
        │       Dispatch Thread (Serial Write)         │
        │  • Processes queued state changes            │
        │  • Generates control output frames           │
        │  • Writes to COM ports (device subscriptions)│
        │  • Collects bidirectional commands from devs │
        └──────────────────────────────────────────────┘
```

### State Management
- **64 KB Shared State Map:** Mirror of DCS-BIOS address space
- **Dirty Tracking:** Per-word bit flags; cleared after dispatch
- **Subscription Filtering:** Per-device address filter sets
- **Command Queue:** Bidirectional device→DCS command buffer

### Data Flow
1. **Inbound:** DCS exports state → Network thread parses → State map updated → Dirty flags set
2. **Dispatch:** Dispatch thread reads dirty addresses → Builds control frames → Writes to subscribed devices
3. **Feedback:** Devices send import commands → Network thread routes to UDP/TCP output to DCS
4. **Logging:** All events queued to UI message buffer (50ms flush timer, 2000-line ring buffer)

---

## Building & Testing

### Prerequisites
- Windows 10 or later
- Visual Studio 2019+ or Visual Studio C++ Build Tools
- CMake 3.20+

### Build Steps
```powershell
cd Programs/dcsbios-serial-bridge
cmake -S . -B build
cmake --build build --config Release
```

### Running Tests
```powershell
# Dry-run mode (network only, no serial writes)
.\build\Release\dcsbios-serial-bridge.exe --dry-run --autostart

# Specific COM ports
.\build\Release\dcsbios-serial-bridge.exe --ports=3,4,5

# TCP mode
.\build\Release\dcsbios-serial-bridge.exe --tcp --autostart
```

### Stress Testing (v0.2+)
```powershell
# Test sequence: idle, switch spam, knob sweep, gauge mode
.\test-stress.ps1
```

---

## Contributing

### Guidelines
1. **Code Quality:** All changes require comments explaining purpose and threading behavior
2. **Testing:** Test locally before submitting (see Testing section above)
3. **License:** All contributions must comply with CC BY-NC-SA 4.0
4. **Communication:** Use Discord or GitHub Issues for discussion before major refactors

### Process
1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature-name`
3. Make changes and commit with clear messages
4. Push to your fork
5. Submit a pull request with detailed description

### Code Organization
```
Programs/dcsbios-serial-bridge/
├── src/
│   ├── main.cpp               # UI thread, message loop, window management
│   ├── BiosProtocol.hpp       # State map, dirty tracking, protocol parsing
│   ├── ControlDatabase.hpp    # JSON loading, control reference metadata
│   ├── DeviceRegistry.hpp     # Per-device handshake, subscriptions, topology
│   └── (future modules for board discovery, panel mapping, etc.)
├── CMakeLists.txt
├── build-package.cmd
├── README.md                  # WIP status (brief)
├── known-bugs.md              # Active issues
├── next-session-plan.txt      # Detailed dev planning
└── DEVELOPMENT.md             # This file
```

---

## Getting Help

### Documentation
- **[README.md](README.md)** — Quick start, features, license
- **[DEVELOPMENT.md](DEVELOPMENT.md)** — This file; status, roadmap, architecture
- **[known-bugs.md](known-bugs.md)** — Current issues and workarounds

### Community & Support
- **OpenHornet Discord:** Primary community channel
- **GitHub Issues:** Report bugs or request features
- **GitHub Discussions:** General Q&A and ideas

---

## Acknowledgments

### Core Contributors
- **Granby** — Original DCS-BIOS bridge concept, Windows GUI implementation

### Key References
- **DCS-BIOS** — Export protocol and state-map design (MIT licensed)
- **OpenHornet** — Handshake protocol inspiration, target integration partner
- **DCS Community** — Testing feedback and collaborative problem-solving

---

## Disclaimer

**This software is provided "AS IS" without warranty.** Users assume all responsibility for:
- Hardware compatibility and proper integration
- Safe operation of physical cockpit systems
- Compliance with third-party licenses (especially OpenHornet's CC BY-NC-SA 4.0 if using their hardware)

**Always test thoroughly in a safe environment before operational use.**

---

**Last Updated:** April 30, 2026  
**Next Roadmap Review:** June 15, 2026
