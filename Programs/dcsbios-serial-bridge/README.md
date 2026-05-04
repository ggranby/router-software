# DCS-BIOS Serial Bridge (WIP)

This is a new standalone Windows GUI replacement for the legacy serial bridge batch scripts.

Current implementation status:
- GUI with mode selection (UDP/TCP)
- COM port entry or auto-detect
- Start/Stop connection button
- F8 hotkey toggles Start/Stop
- Resizable dark-themed UI with responsive layout
- Export Log button (saves session log to UTF-8 .txt)
- Single-process fanout to multiple COM ports
- Dry-run smoke-test mode (network only, no serial writes)
- Periodic packet count logging to verify data reception
- Startup flags for wrapper compatibility (`--autostart`, `--udp`, `--tcp`, `--ports=...`, `--dry-run`)

## Build (Visual Studio C++ Build Tools)

From this folder:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Or use the helper script:

```bat
build-package.cmd
```

Installer/package helper (CPack):

```bat
make-installer.cmd ZIP
```

If NSIS is installed, you can also generate a Windows installer:

```bat
make-installer.cmd NSIS
```

Optional configuration argument:

```bat
build-package.cmd Debug
```

Binary output:
- `build/Release/hornet-link.exe`
- `build/package/hornet-link-Release/hornet-link.exe`
- `build/package/hornet-link-Release/json/` (bundled DCS-BIOS control database when found)
- `dist/hornet-link-Release.zip` (from helper script)

Release package contents currently include:
- `hornet-link.exe`
- `device_profiles.json`
- `quick-start.cmd`
- `README.md`
- `LICENSE` (when available)
- `json/` control database folder (when found)

## Optional Dear ImGui Prototype

The current production UI is Win32. An optional Dear ImGui prototype target is
available for migration work.

```bat
setup-imgui.cmd
cmake -S . -B build -DHORNET_LINK_ENABLE_IMGUI=ON
cmake --build build --config Release --target hornet-link-imgui
```

Or via helper:

```bat
quick-start.cmd --imgui
```

ImGui prototype output:
- `build/Release/hornet-link-imgui.exe`

## Runtime defaults

- UDP mode listens on `0.0.0.0:5010` and joins multicast `239.255.50.10`
- TCP mode connects to `127.0.0.1:7778`
- Serial ports are configured to `250000 8N1`
- The bridge looks for control-reference JSON in this order: `json/` beside the executable, then generated DCS folders under `Saved Games`, then repository-relative doc folders
- `build-package.cmd` bundles generated control-reference JSON from `Saved Games\DCS\Scripts\DCS-BIOS\doc\json` when available

## Startup flags

- `--autostart` starts bridging as soon as the window opens
- `--udp` forces UDP mode
- `--tcp` forces TCP mode
- `--ports=3,4,5` preloads COM port list
- `--dry-run` receives network data and logs packet flow without opening/writing COM ports
- `--imgui` (quick-start only) builds/launches the optional Dear ImGui prototype target
- positional argument `3` is treated as legacy single COM selection compatibility

## Notes

This folder is intentionally isolated from the rest of the repository to minimize impact while development is in progress.

The bridge automatically retries socket initialization and reconnects after socket errors with bounded backoff.
