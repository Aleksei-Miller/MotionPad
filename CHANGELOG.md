# Changelog

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
