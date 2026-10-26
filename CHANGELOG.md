# Changelog

## [Unreleased]

### Added

- **OEM symbol keys in keyboard bindings** — `k:` accepts `backtick`, `minus`, `equals`, `lbracket`, `rbracket`, `backslash`, `semicolon`, `apostrophe`, `comma`, `period`, `slash` and `oem102`, with a single name per key.
- **`none mode` profile** — Fifth pre-built profile with every binding empty, shipped with builds and ordered under `controller mode` in the tray menu.
- **Service → Reconnect Devices** — Manual device rescan; Disable Emulation moved into the Service menu with a dynamic label.
- **Auto-repeat opt-out** — Negative `[Keyboard] Key=-1` / `[Mouse] Key=-1` holds the key or button until release instead of repeating.

### Changed

- **PS Move launch** — `psmove.exe` is started via `cmd /k` so its console output stays readable.
- **Tray Service menu order** — Reconnect/Toggle placed before Calibrate/Pair.

### Fixed

- **Button-bound analog actions did nothing** — `updateDigitalButtonAction` had no cases for `ActionKind_XboxTrigger`, `ActionKind_XboxAxis*` and `ActionKind_MouseMove*`, so `CROSS=x:lt` and `CROSS=m:x+` fell through to `default`. Digital buttons now emit full deflection (`255`, `±32767`) and move the mouse by a fixed step while held; `TRIGGER=` stays analog.
- **Trigger axis zeroed by a second device** — Analog `TRIGGER→x:lt`/`x:rt` only raises the axis value, so a second connected Move or Navigation with a released trigger no longer overwrites `bLeftTrigger`/`bRightTrigger`.
- **Silent Move not dropped** — A Bluetooth Move that stops delivering packets is disconnected by the exact device after `MOVE_SILENCE_TIMEOUT_MS` instead of guessing a slot from `psmove_count_connected()`.
- **Already-connected Move reopened** — The service tick no longer reopens a Move that is already connected.
- **Missing `default.ini`** — Falls back to `controller mode.ini` instead of failing to load.

### Optimized

- **Device discovery** — Driven by `WM_DEVICECHANGE`/`DBT_DEVNODES_CHANGED` with a 2000 ms fallback instead of a full enumeration every 250 ms, so a freshly powered-on controller is picked up on the next frame.

### Removed

- **`libnavinput` Navigation backend** — SDL3 is now the only backend for PS Navigation controllers (USB/BT via BthPS3 + DsHidMini). `UseBthPS3` in `settings.ini` removed, `NavBackend`/`NavDeviceType` enums and `navDeviceGetType()` removed.

## [0.1.1]

### Added

- **SDL3 Bluetooth Navigation backend** — New `NavDevice` abstraction layer (`nav_device.c/h`) supporting dual backends: `libnavinput` (USB) and `SDL3` (Bluetooth via BthPS3 + DsHidMini). Selected by `UseBthPS3` in `settings.ini`.
- **Auto-profile switching** — `trayCheckAutoProfile()` detects foreground window executable name and auto-switches to a matching `[Profile] Path=`. Toggle via `UseAutoProfile=1` in `settings.ini`.
- **Settings.ini runtime watcher** — Filesystem watcher (`settingsWatcherStart/Stop`) detects changes to `settings.ini` at runtime and reloads settings without restart.
- **Device liveness polling** — `deviceCheckAlive()` runs every 3 seconds, calls `psmove_poll()` per Move + `navDevicePoll()` per Nav to detect OS-side disconnections.
- **Navigator cooldown** — Prevents rapid reconnect loops on disconnect/reconnect with per-navigator cooldown paths and timestamps.
- **Configurable poll rate** — Reads `PollRateMs` from `settings.ini` (default 12ms). Old version hardcoded 10ms.
- **`[Mouse] Key=`** — Separate repeat rate for mouse button actions (alongside existing `[Keyboard] Key=`). Read by `profileReadMouseKey()`.
- **Auto-repeat for button-triggered keyboard/mouse** — `updateDigitalButtonAction` now auto-repeats `ActionKind_Keyboard` and `ActionKind_MouseButton` at `[Keyboard] Key=` / `[Mouse] Key=` intervals.
- **High priority process class** — `SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS)`.
- **Single instance mutex** — Renamed from `PSMove4PC` to `MotionPad`.
- **Four pre-built profiles** (was two): `hybrid mode.ini`, `controller mode.ini`, `wasd mode.ini`, `arrow mode.ini`.

### Changed

- **Processing order** — Nav processed before Move (Move accelerometer can overwrite Navigator on shared axes).
- **Navigator types** — `PSNavigator*` replaced with `NavDevice*` throughout (abstracted backend).
- **Profile section names** — `[Navigator*]` → `[Navigation*]`, `[Misc]` → `[Profile]`.
- **Repeat rate sections** — Old `[RepeatMs]` with `Key=/Wheel=` split into `[Keyboard] Key=`, `[Mouse] Key=`, `[Mouse] Wheel=`.
- **Telemetry** — Reads cached battery/state instead of calling `psmove_poll()` (no poll drain).
- **Battery reading** — Cached in `move_battery_raw[]` after move processing instead of direct `psmove_get_battery()`.
- **Output toggle** — `emulation_enabled` renamed to `output_enabled`.
- **`[General] EnableEmulation`** — Renamed to `[General] EnableOutput`.

### Optimized

- **`GetTickCount()`** — Called once per frame instead of 22 times (<code>68e8163</code>).
- **`psmove_count_connected()`** — Cached in `AppContext.move_count_cache`, updated every 250ms (<code>a89a236</code>).
- **`SDL_PumpEvents()`** — Reduced from 4× to 1× per frame via `navDeviceFrameStart()` (<code>60be138</code>).
- **`trayCheckAutoProfile()`** — Throttled to once per 200ms (<code>a974d08</code>).
- **`roundf`/`pow`** — Skipped when sensitivity == 1.0 (<code>5aacf61</code>).
- **`psmove_poll` drain** — Bounded to 32 iterations per tick (was unbounded) (<code>5aacf61</code>).
- **Empty sensor bindings** — Skipped (actionCount guard) (<code>5aacf61</code>).
- **`deviceCheckAlive()`** — Removed redundant nav poll (navigators already polled per-frame) (<code>5aacf61</code>).
- **`Sleep(100)` → `Sleep(250)`** — On ViGEm error, for consistency with `WAIT_SLEEP_MS` (<code>5aacf61</code>).

### Fixed

- `Sleep(100)` on ViGEm error inconsistency (changed to `WAIT_SLEEP_MS`).
- Device count cache updated every frame (moved inside 250ms service poll, <code>8fe7e2a</code>).
