# Hornet Link

A Windows GUI application for bridging DCS-BIOS export streams to Arduino-based cockpit panels and hardware interfaces via serial COM ports. Designed to integrate custom F/A-18C Hornet cockpit systems with DCS World.

## License

This project is licensed under **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)**.

- **You are free to:** Use, modify, and distribute this software
- **Under these conditions:**
  - **Attribution:** Credit must be given to Hornet Link
  - **Non-Commercial:** Cannot be used for commercial purposes without explicit written permission
  - **ShareAlike:** Any derivatives must be licensed under the same CC BY-NC-SA 4.0 license

For full license details, see [LICENSE](LICENSE) or visit http://creativecommons.org/licenses/by-nc-sa/4.0/

### Commercial Use

To request commercial licensing or alternative terms, please contact the project maintainers.

## OpenHornet Compatibility & Integration

Hornet Link is designed to work seamlessly with [OpenHornet](https://www.openhornet.com/) F/A-18C simulator projects. It provides the critical software bridge between the OpenHornet physical cockpit hardware and DCS World.

**OpenHornet © 2015-2026** is licensed under CC BY-NC-SA 4.0. See https://www.openhornet.com/ for details.

When using Hornet Link with OpenHornet hardware or design elements, ensure compliance with OpenHornet's CC BY-NC-SA 4.0 license terms.

## Features

- **Dual Protocol Support:** UDP multicast (DCS default) and TCP connections
- **Multi-Port Management:** Fan out to multiple COM ports simultaneously
- **Smart Handshaking:** Negotiates device topology (standalone/master/slave) and subscription preferences
- **Bidirectional Support:** Devices can send import commands back to DCS
- **Dry-Run Mode:** Test network connectivity without serial writes
- **Dark Theme GUI:** Resizable, responsive Windows interface
- **Session Logging:** Export packet flow and state changes to UTF-8 text files
- **Startup Automation:** Command-line flags for headless/automated operation

## Quick Start

### Build Requirements

- Windows 10 or later
- Visual Studio C++ Build Tools (or full Visual Studio)
- CMake 3.20+

### Build Instructions

```powershell
cd Programs/dcsbios-serial-bridge
cmake -S . -B build
cmake --build build --config Release
```

Or use the helper script:

```batch
build-package.cmd
```

Binary output:
- `build/Release/dcsbios-serial-bridge.exe`
- `build/package/Release/` (with bundled control reference JSON)
- `dist/dcsbios-serial-bridge-Release.zip` (release package)

### Usage

```powershell
dcsbios-serial-bridge.exe [options]

Options:
  --autostart              Start bridging immediately on launch
  --udp                    Force UDP mode (default)
  --tcp                    Force TCP mode
  --ports=3,4,5            Preload COM ports (space, comma, or semicolon separated)
  --dry-run                Receive network data but don't write to COM ports
```

### Configuration

- **UDP Mode (Default):** Listens on `0.0.0.0:5010`, joins multicast `239.255.50.10:5010`
- **TCP Mode:** Connects to `127.0.0.1:7778`
- **Serial Speed:** 250000 baud 8N1 (configurable in source)
- **Control Database:** Searches for DCS-BIOS JSON in:
  - `json/` folder beside executable
  - `%USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\doc\json`
  - `%USERPROFILE%\Saved Games\DCS.openbeta\Scripts\DCS-BIOS\doc\json`

## Device Handshake Protocol

### Probe Frame (Bridge → Device)

```
0xAA 0xDE 0xAD 0x01
```

### Response (Device → Bridge)

```
0xAA 0xDE 0xAD 0x02 [flags] [name_len] [name:N] [sub_count:2] [subscriptions...]
```

**Flags:**
- `0x01` = RS-485 Master (has downstream slaves)
- `0x02` = RS-485 Slave (managed by another master)
- `0x04` = Bidirectional (can send import commands)

**Subscriptions:** Each is `[addr:2] [mask:2] [shift:1]` (6 bytes). Use `0xFFFF 0xFFFF 0x00` for "all".

## Architecture

The bridge uses a multi-threaded architecture:
- **UI Thread:** Windows message loop and user interaction
- **Network Thread:** UDP/TCP socket management and protocol parsing
- **Dispatch Thread:** State tracking and COM port writes

Communication is thread-safe via atomic counters and message queues.

## Dependencies

### DCS-BIOS (Required for full cockpit data)

Hornet Link relies on [DCS-BIOS](https://github.com/DCS-Skunkworks/dcs-bios) to
receive cockpit state data from DCS World. DCS-BIOS must be installed in your DCS
Saved Games folder for panel indicators, switches, and gauges to update.

**Without DCS-BIOS installed:**
- `HornetLinkExport.lua` falls back to a minimal synthetic heartbeat that only
  sends aircraft altitude (address 0x0000) at ~30 Hz.
- Panel displays and indicators will **not** update.
- The bridge will still connect and forward frames, but panels receive no useful data.

**Installation:**
1. Download DCS-BIOS from https://github.com/DCS-Skunkworks/dcs-bios/releases
2. Extract to `%USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\`
3. Add to your `Export.lua` as described in the DCS-BIOS README.
4. Optionally also load `HornetLinkExport.lua` for the direct UDP path to Hornet Link.

> **Note:** The DCS-BIOS UDP multicast stream (239.255.50.10:5010) and the direct
> Hornet Link UDP path (127.0.0.1:42002 via `HornetLinkExport.lua`) are both
> supported. The direct path has lower latency but requires the Lua file installed.

### Lua Exporter (`HornetLinkExport.lua`) — Optional but Recommended

The included Lua exporter provides a dedicated low-latency path from DCS to Hornet Link,
bypassing the multicast network stack. It hooks into DCS-BIOS's `ExportReceiveData`
callback and forwards each frame directly over UDP to `127.0.0.1:42002`.

- If DCS-BIOS is present, the exporter piggybacks on the DCS-BIOS stream.
- If DCS-BIOS is **not** present, a minimal fallback frame is synthesised using
  `LoGetSelf()` (altitude only). This is sufficient to verify connectivity but
  not suitable for panel operation.

## Known Limitations

- Windows-only (uses Windows API for COM ports and sockets)
- Single DCS instance per bridge instance
- No automatic reconnection to DCS on network restart (manual restart required)
- **DCS-BIOS required** for full cockpit panel data (see Dependencies above)

## Contributing

Contributions are welcome under the CC BY-NC-SA 4.0 license. 

Before submitting:
1. Fork the repository
2. Create a feature branch
3. Test your changes thoroughly
4. Submit a pull request with clear description

All contributions must comply with CC BY-NC-SA 4.0 terms.

## Acknowledgments

- **DCS-BIOS:** Original export protocol and state-map design (MIT license)
- **OpenHornet:** Inspiration for device handshake and subscription architecture; test platform
- **DCS Community:** Feedback, testing, and collaborative development

## Disclaimer

This software is provided "AS IS" without warranty. Users assume all responsibility for:
- Hardware compatibility and proper operation
- Safe integration with physical cockpit systems
- Compliance with all applicable licenses when used with third-party projects

Always test thoroughly in a safe environment before operational use.

---

**Hornet Link** — Bridging the gap between your cockpit and the sky.
