# AGENTS.md

## Build

```powershell
cmake --build app/build/gcc          # MinGW GCC
cmake --build app/build/tcc          # TCC
cmake --build app/build/msvc         # MSVC
```

Run from `app/` directory. The binary appears in `build/<compiler>/bin/motionpad.exe`.

## Project Structure

```
app/
├── src/            # C source files
├── include/        # Headers (psmoveapi, ViGEm, SDL3)
├── lib/            # Import libs (.def files, .a)
├── runtime/        # DLLs, settings.ini, default profiles
├── build/          # Out-of-source CMake builds (gcc/, tcc/, msvc/)
├── tests/          # Unit tests (standalone, not CMake-integrated)
├── PROFILE_REFERENCE.md
└── AGENTS.md       # This file
```

## Key Source Files

| File | Purpose |
|------|---------|
| `main.c` | Main loop: poll devices → process input → submit Xbox report |
| `profile.c` | INI parsing: loads all sections for Move + Navigation |
| `input_actions.c` | Action execution: Xbox buttons/axes, keyboard, mouse, internal |
| `device_manager.c` | Connect/disconnect detection for Move + Navigation |
| `move_input.c` | PS Move polling + binding processing |
| `navigator_input.c` | PS Navigation binding processing |
| `nav_device.c` | Navigation device abstraction (libnavinput / SDL3 backends) |
| `vigem_manager.c` | ViGEmBus client: create/destroy/submit Xbox 360 pad |
| `tray_ui.c` | System tray icon, tooltip (battery), menu |
| `profile_watcher.c` | Filesystem watcher for live profile reload |
| `rumble_manager.c` | Forwards Xbox rumble to PS Move motors |
| `telemetry.c` | Named pipe streaming of controller state |
| `logger.c` | Debug logging |

## Architecture / Data Flow

```
Main loop (every ~9ms, PollRateMs in settings.ini):
  1. trayApplyPendingProfileSelection — loads queued profile switch
  2. trayCheckAutoProfile — detects foreground window, auto-switches profile if [Profile] Path= matches
  3. updateServiceDevices (every 3s) — connects new Moves/Navigators, removes OS-disconnected
  3. updateTrayTooltipIfNeeded — reads *cached* battery, no psmove_poll
  4. deviceCheckAlive (every 3s) — single psmove_poll per Move, navDevicePoll per Nav
  5. appContextZeroReport — zeros XUSB_REPORT for new frame
  6. processNavigatorInputDevice — poll Nav, apply bindings → XUSB_REPORT
  7. processMoveInputDevice — while(psmove_poll), apply button + sensor bindings → XUSB_REPORT
  8. maybeSendTelemetry — reads cached state, no poll drain
  9. vigemSubmitReport — sends XUSB_REPORT to ViGEmBus (virtual Xbox 360 pad)
```

## Important Patterns

### `isDown` tracking for Xbox buttons
Digital button actions (`updateDigitalButtonAction`) do NOT track `isDown` for Xbox buttons — they always set the bit and rely on `appContextZeroReport` to clear it next frame. Sensor-triggered button actions (`applySensorButtonLikeAction`) DO track `isDown` to manage press/release state and clear their bit on release. This prevents interference between devices sharing the same binding.

### Auto-repeat for digital button actions
`updateDigitalButtonAction` supports auto-repeat for `ActionKind_Keyboard` and `ActionKind_MouseButton`. When held, the key/mouse button repeats at the interval set by `[Keyboard] Key=` / `[Mouse] Key=` in the profile INI. `isDown` and `lastRepeatTick` are tracked per-action to manage press, repeat, and release. Mouse wheel actions also repeat using `[Mouse] Wheel=`.

### Report sharing
A single `XUSB_REPORT` is shared across all connected devices. Processing order is Nav→Move so Move accelerometer can overwrite Navigator on shared axes. Bits not explicitly cleared will persist until `appContextZeroReport` at the start of the next frame.

### Poll draining
`psmove_poll()` drains one packet per call. `processMoveInputDevice` uses `while (psmove_poll)` to drain all available packets, ensuring the getters return the latest state. `deviceCheckAlive` and `maybeSendTelemetry` do NOT drain — they read cached state. Battery is cached after `processMoveInputDevice` so tray doesn't need to poll.

### Navigation backends
Two backends, selected by `UseBthPS3` in `settings.ini`:
- `libnavinput` — USB-only, WinUSB driver, battery via `psnavigatorGetBattery()`
- `SDL3` — USB/BT, requires BthPS3 + DsHidMini mode 3, battery via `HidD_GetFeature` 50-byte report (byte[31]: 0x00–0x05 = battery, 0xEE/0xEF = USB)

## Auto-Profile Switching

When `UseAutoProfile=1` in `settings.ini`, the app reads `[Profile] Path=` from every `.ini` in the `profiles/` directory. On each frame it gets the foreground window's executable name via `QueryFullProcessImageNameW` and matches it (case-insensitive, basename only) against each profile's `Path` value. On match, it queues a profile switch.

## Profile Sections

All `Navigator` section names use `Navigation`:
- `[Navigation1]`, `[Navigation2]`
- `[NavigationTrigger1]`, `[NavigationTrigger2]`
- `[NavigationStick1]`, `[NavigationStick2]`
- `[AltNavigation1]`, `[AltNavigation2]`
- `[AltNavigationTrigger1]`, `[AltNavigationTrigger2]`
- `[AltNavigationStick1]`, `[AltNavigationStick2]`

## Constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `MOVE_COUNT` | 2 | Max PS Move controllers |
| `NAVIGATOR_COUNT` | 2 | Max PS Navigation controllers |
| `POLL_RATE_MS` | 4 (default) | Main loop sleep between iterations |
| `DEVICE_LIVENESS_POLL_MS` | 3000 | How often to check device liveness |
| `TRAY_UPDATE_INTERVAL_MS` | 1000 | How often to refresh tray tooltip |

## Code Conventions

- No comments in code. Express intent through naming.
- C99 standard.
- Windows-only, Win32 API.
- Static functions where possible.
- `_snwprintf` / `GetPrivateProfileIntW` / `GetPrivateProfileSectionW` for config.
- `XUSB_REPORT` shared across all input devices.
- Hungarian notation not used.
