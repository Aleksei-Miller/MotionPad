# MotionPad

`MotionPad` is a Windows tray app that turns PS Move and PS Navigation controllers into an Xbox 360 controller, keyboard and mouse input.

It is designed to stay in the background, reload profiles without restarting, and let you switch control schemes quickly from the tray.

### What You Get

- Gyroscope and accelerometer mapping for PS Move
- Rumble forwarding to PS Move controllers
- Mapping to Xbox 360 controller, keyboard, and mouse output from a single profile
- Live profile reload without restarting the app
- Alternate layers through `alt_mode`

### System requirements:

- [ViGEmBus](https://github.com/nefarius/ViGEmBus) 
- [Zadig](https://zadig.akeo.ie) (required for PS Navigation WinUSB driver installation)

### PS Navigation Setup (Mandatory)

If you are using a **PS Navigation** controller, you must manually replace its driver with **WinUSB** to make it visible to MotionPad:

1. Connect the Navigation Controller via USB.
2. Run **Zadig** as Administrator.
3. Select `Options` -> `List All Devices`.
4. Select **Navigation Controller** from the dropdown list.
5. Set the target driver to **WinUSB** (using the arrows).
6. Click **Replace Driver**.
7. Once finished, the device should appear under "Universal Serial Bus devices" in Device Manager.

### How To Start

Launch `motionpad.exe`. The app appears in the system tray and keeps running in the background.

When emulation is enabled and at least one controller is connected, MotionPad creates a virtual Xbox 360 controller through ViGEmBus.

### Tray Menu

From the tray you can:

- See controller battery summary
- Enable or disable emulation
- Pair PS Move controllers 
- Launch PS Move calibration
- Switch the active profile

### Controller Notes

Limits:

- Up to 2 PS Move controllers
- Up to 1 PS Navigation controller over wired USB only

### Logs

MotionPad writes runtime information to `motionpad.log`.

### Binding syntax
Profile sections, field names, binding syntax, and action prefixes are documented in:

- `PROFILE_REFERENCE.md`

### Project Layout

- `src/` - application sources
- `include/` - bundled headers
- `lib/` - `.def` files and generated import libraries
- `runtime/` - runtime DLLs and default config
- `runtime/settings.ini` - startup settings
- `runtime/profiles/default.ini` - sample profile
- `PROFILE_REFERENCE.md` - profile format and action reference
- `CMakeLists.txt` - build configuration

### Build

#### GCC / MinGW

```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

Output:

- `build\bin\motionpad.exe`

#### TinyCC

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=tcc -DCMAKE_RC_COMPILER=windres
cmake --build build
```

#### MSVC

```bash
cmake -S . -B build -G "Visual Studio 16 2019" -A Win32
cmake --build build --config Release
```

Notes:

- `GCC` build requires `cmake`, `gcc`, and `dlltool`
- `TinyCC` build still requires MinGW tools
- `MSVC` build requires Visual Studio Build Tools
- Runtime DLLs and default config are copied from `runtime/`

### Architecture Notes

- Windows-only app
- Manual message loop via `PeekMessageW`
- Config reload via file watching
- PS Move support delegated to `psmoveapi`
- PS Navigation support delegated to `libnavinput`
- Xbox 360 controller emulation delegated to `ViGEmClient`

## Trademark notice

This software is an independent, open-source project and is **not** affiliated with, authorized, maintained, sponsored, or endorsed by Sony Interactive Entertainment Inc., Microsoft Corporation, or any of their affiliates.

"PlayStation", "PS Move", "PS Navigation", "DUALSHOCK" and related marks are registered trademarks of Sony Interactive Entertainment Inc. All Sony controller names, images, and references in this repository are used strictly for **nominative purposes** — only to identify hardware compatibility and provide instructions to the user.

"Xbox", "Xbox 360", and "Windows" are registered trademarks of Microsoft Corporation in the United States and/or other countries. Any Microsoft product names are used only to describe software and hardware compatibility.

All other trademarks, logos, and brands are the property of their respective owners. The use of these names, logos, and brands does not imply endorsement.

## License

MotionPad is open-source software licensed under the **GNU General Public License v3.0 (GPLv3)**. 

You are free to use, modify, and distribute this software under the terms of this license. For more information, please see the `LICENSE` file in the repository.

## Built With

* [PS Move API](https://github.com/thp/psmoveapi) — Licensed under the [Simplified BSD License](https://github.com/thp/psmoveapi/blob/master/COPYING).
* [ViGEmBus](https://github.com/nefarius/ViGEmBus) — Licensed under the [BSD 3-Clause](https://github.com/nefarius/ViGEmBus/blob/master/LICENSE).

### Credits
* Developed via ChatGPT (Prompt-based development).
