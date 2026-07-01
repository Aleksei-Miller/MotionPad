# MotionPad

`MotionPad` is a Windows tray app that turns PS Move and PS Navigation controllers into an Xbox 360 controller, keyboard, and mouse input.

It is designed to stay in the background, reload profiles without restarting, and let you switch control schemes quickly from the tray.

### What You Get

- Gyroscope and accelerometer mapping for PS Move
- Rumble forwarding to PS Move controllers
- Mapping to Xbox 360 controller, keyboard, and mouse output from a single profile
- Live profile reload without restarting the app
- Alternate layers through `alt_mode`
- Auto-profile switching based on foreground window (see `UseAutoProfile` in `runtime/settings.ini`)

## System requirements:

- [ViGEmBus](https://github.com/nefarius/ViGEmBus)
- [Zadig](https://zadig.akeo.ie) (required for PS Navigation WinUSB driver installation)
- [BthPS3](https://github.com/nefarius/BthPS3) + [DsHidMini v2.2.282](https://github.com/nefarius/dshidmini) (optional, for PS Navigation over Bluetooth)

## PS Navigation Setup

MotionPad supports two backends for PS Navigation controllers, selected at startup via `UseBthPS3` in `runtime/settings.ini`:

| Backend | Connection | Driver |
|---------|-----------|--------|
| `libnavinput` (default, `UseBthPS3=0`) | USB only | WinUSB via Zadig |
| SDL3 (`UseBthPS3=1`) | USB and Bluetooth | BthPS3 + DsHidMini |

#### Driver Setup — Wired USB (libnavinput)

1. Connect the Navigation Controller via USB.
2. Run **Zadig** as Administrator.
3. Select `Options` -> `List All Devices`.
4. Select **Navigation Controller** from the dropdown list.
5. Set the target driver to **WinUSB** (using the arrows).
6. Click **Replace Driver**.
7. Once finished, the device should appear under "Universal Serial Bus devices" in Device Manager.

#### Driver Setup — Bluetooth (BthPS3)

For wireless use with the BthPS3 backend, set `UseBthPS3=1` in `runtime/settings.ini`.

1. Remove all Bluetooth devices from Windows Bluetooth settings
2. Install [BthPS3](https://github.com/nefarius/BthPS3)
3. **Reboot**
4. Download [DsHidMini v2.2.282](https://github.com/nefarius/dshidmini)
5. Prepare `dshidmini.inf`:
   - Uncomment lines 63 and 65 (enable Navigation Controller hardware IDs)
   - On line 129, change the last value from `5` to `3` (DS3-compatible mode instead of XInput)
   - Install the modified driver via [fawazahmed0/windows-unsigned-driver-installer](https://github.com/fawazahmed0/windows-unsigned-driver-installer)
6. **Reboot**
7. Run **BthPS3 Driver Configuration Utility**, go to *Profile Driver Settings*, enable **"Enable PlayStation Move Navigation Support"**, leave **"Enable PlayStation Move Motion Support"** disabled
8. Connect PS Navigation via USB to verify detection
9. Disconnect PS Navigation from USB
10. Press the **PS** button on the controller — it appears as an unknown device
11. In Device Manager, assign the dshidmini driver manually → select **"Navigation Compatible HID Device"**
12. The controller is now ready. It connects automatically when the PS button is pressed.

## How To Start

Launch `motionpad.exe`. The app appears in the system tray and keeps running in the background.

When emulation is enabled and at least one controller is connected, MotionPad creates a virtual Xbox 360 controller through ViGEmBus.

## Tray Menu

From the tray you can:

- See controller battery summary
- Enable or disable emulation
- Pair PS Move controllers 
- Launch PS Move calibration
- Switch the active profile

## Auto-Profile Switching

MotionPad can switch profiles automatically based on the foreground window. Enable it in `runtime/settings.ini`:

```ini
UseAutoProfile=1
```

Then set `Path=` under `[Profile]` in each profile's `.ini` file to match a process name:

```ini
[Profile]
Path=C:\Program Files\Game\game.exe
Name=Some game
```

Only the basename is matched (case-insensitive), so `game.exe` works even with a full path.

## Controller Notes

#### Limits:

- Up to 2 PS Move controllers
- Up to 2 PS Navigation controllers (USB or Bluetooth)

#### Logs

MotionPad writes runtime information to `motionpad.log`.

### Binding syntax
Profile sections, field names, binding syntax, and action prefixes are documented in:

- `PROFILE_REFERENCE.md`

## Project Layout

- `src/` - application sources
- `include/` - bundled headers
- `include/SDL3/` - SDL3 headers (for SDL3 backend)
- `lib/` - `.def` files and import libraries
- `lib/SDL3/` - SDL3 import library and DEF file
- `runtime/` - runtime DLLs and default config
- `runtime/settings.ini` - startup settings
- `runtime/profiles/` - built-in profiles (hybrid mode.ini, controller mode.ini, arrow mode.ini, wasd mode.ini)
- `PROFILE_REFERENCE.md` - profile format and action reference
- `CMakeLists.txt` - build configuration

## Build

All compilers use a single `cmake --build` command after configuration. Run from the `app/` directory.

#### GCC / MinGW

```bash
cmake -S . -B build/gcc -G "MinGW Makefiles"
cmake --build build/gcc
```

#### TinyCC

```bash
cmake -S . -B build/tcc -G "MinGW Makefiles" -DCMAKE_C_COMPILER=tcc -DCMAKE_RC_COMPILER=windres
cmake --build build/tcc
```

#### MSVC

```bash
cmake -S . -B build/msvc -G "Visual Studio 16 2019" -A Win32
cmake --build build/msvc --config Release
```

Notes:

- `GCC` build requires `cmake`, `gcc`, and `dlltool`
- `TinyCC` build requires MinGW tools for `windres` (the RC compiler)
- `MSVC` build requires Visual Studio Build Tools
- Runtime DLLs and default config are copied from `runtime/` during build

### Architecture Notes

- Windows-only app
- Manual message loop via `PeekMessageW`
- Config reload via file watching
- PS Move support delegated to `psmoveapi`
- PS Navigation support via two backends (selected at startup via `UseBthPS3` in `runtime/settings.ini`):
  - `libnavinput` — USB-only.
  - SDL3 — USB and Bluetooth (via BthPS3 + DsHidMini).
- Poll rate controlled via `PollRateMs` in `runtime/settings.ini` (1-100 ms, hot-reloadable)
- Telemetry streams controller state to `\\.\pipe\motionpad` when `EnableTelemetry=1` in `runtime/settings.ini`
- Xbox 360 controller emulation delegated to `ViGEmClient`

## Trademark notice

This software is an independent, open-source project and is **not** affiliated with, authorized, maintained, sponsored, or endorsed by Sony Interactive Entertainment Inc., Microsoft Corporation, or any of their affiliates.

"PlayStation", "PS Move", "PS Navigation", "DUALSHOCK" and related marks are registered trademarks of Sony Interactive Entertainment Inc. All Sony controller names, images, and references in this repository are used strictly for **nominative purposes** — only to identify hardware compatibility and provide instructions to the user.

"Xbox", "Xbox 360", and "Windows" are registered trademarks of Microsoft Corporation in the United States and/or other countries. Any Microsoft product names are used only to describe software and hardware compatibility.

All other trademarks, logos, and brands are the property of their respective owners. The use of these names, logos, and brands does not imply endorsement.

## Built With

* [PS Move API](https://github.com/thp/psmoveapi) — Licensed under the [Simplified BSD License](https://github.com/thp/psmoveapi/blob/master/COPYING).
* [ViGEmBus](https://github.com/nefarius/ViGEmBus) — Licensed under the [BSD 3-Clause](https://github.com/nefarius/ViGEmBus/blob/master/LICENSE).
* [Simple DirectMedia Layer](https://github.com/libsdl-org/SDL) — Licensed under the [Zlib license](https://github.com/libsdl-org/SDL/blob/main/LICENSE.txt).

### Credits
* Developed via ChatGPT, DeepSeek (Prompt-based development).