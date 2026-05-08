# Profile Reference

This document describes the MotionPad profile format, supported sections, field names, value formats, and action prefixes.

## Overview

MotionPad reads `settings.ini` from the executable directory. By default, it loads `profiles\default.ini`.

`settings.ini` contains global application settings:

Section`[General]`

| Name | Description | Value |
| --- | --- | --- |
| `Path` | Active profile path | Relative path |
| `Enable` | Enable or disable emulation | `0` or `1` |

A profile file such as `profiles\default.ini` contains controller mappings and runtime settings.

## Binding Prefixes

### `x:` Xbox

Sends input to the virtual Xbox 360 controller.



Description | Value |
| --- | --- |
| Buttons |`a`, `b`, `x`, `y`, `back`, `start`, `l3`, `r3`, `guide`|
| Bumpers(left - *lb*, right - *rb*) | `lb`, `rb` |
| D-pad | `up`, `down`, `left`, `right` |
| Triggers(left - *lt*, right - *rt*) | `lt`, `rt` |
| Left stick X | `lx`, `lx+`, `lx-` |
| Left stick Y | `lx`, `lx+`, `lx-` |
| Right stick X | `rx`, `rx+`, `rx-` |
| Right stick Y | `ry`, `ry+`, `ry-` |

*Notes:*
- *`lx` includes `lx+` and `lx-`*
- *`ly` includes `ly+` and `ly-`*
- *`rx` includes `rx+` and `rx-`*
- *`ry` includes `rx+` and `rx-`*
  
Examples:

- `x:a`
- `x:rt`
- `x:lx`
- `x:ry-`

### `m:` Mouse

Sends a mouse action.

| Description | Value |
| --- | --- |
| Buttons(left - *lb*, right - *rb*, middle - *mb*) | `lb`, `rb`, `mb`, `x1`, `x2` |
| Mouse wheel up/down | `mw`, `mw+`, `mw-` |
| Movement X | `x`, `x+`, `x-` |
| Movement Y | `y`, `y+`, `y-` |

*Notes:*
- *`x` includes `x+` and `x-`*
- *`y` includes `y+` and `y-`*

Examples:

- `m:x`
- `m:y-`
- `m:lb`

### `k:` Keyboard

Sends a keyboard key.

Description | Value |
| --- | --- |
| Letters | `a-z` |
| Digits | `0-9` |
| Function keys | `f1-f12` |
| Control keys | `enter`, `return`, `space`, `tab`, `esc`, `escape`, `backspace`, `back` |
| Navigation keys | `left`, `right`, `up`, `down`, `home`, `end`, `pageup`, `pagedown`, `pgup`, `pgdn`, `insert`, `delete`, `ins`, `del`
| Modifiers | `shift`, `ctrl`, `control`, `alt`, `lshift`, `rshift`, `lctrl`, `rctrl`, `lcontrol`, `rcontrol`, `lalt`, `ralt` |
| Lock/system keys | `capslock`, `numlock`, `scrolllock`, `printscreen`, `prtsc`, `pause`, `apps`, `menu`, `win`, `lwin`, `rwin` |
| Numpad keys | `numpad0-numpad9`, `add`, `subtract`, `multiply`, `divide`, `decimal` |

Examples:

- `k:a`
- `k:enter`
- `k:space`
- `k:f1`
- `k:left`

### `o:` Internal

Calls an internal MotionPad action.

Description | Value |
| --- | --- |
| Temporarily switches to `Alt*` sections while held | `alt_mode` | 
| Blocks all gyroscope axes | `gyro_stop_all` |
| Blocks gyroscope X | `gyro_stop_x` |
| Blocks gyroscope Y | `gyro_stop_y` |
| Blocks gyroscope Z | `gyro_stop_z` |
| Captures the current accelerometer position as center for all axes | `accel_reset_all` |
| Captures the current accelerometer position as center for X | `accel_reset_x` |
| Captures the current accelerometer position as center for Y | `accel_reset_y` |
| Captures the current accelerometer position as center for Z | `accel_reset_z` |

Examples:

- `o:alt_mode`
- `o:gyro_stop_x`
- `o:accel_reset_all`

## Notes

- Multiple actions can be assigned on one line, separated by spaces.
- `alt_mode` activates alternate bindings while the assigned control is held.
- `profiles\default.ini` is the canonical example profile.


## Supported Profile Sections

### Main Sections

| Section | Description |
| --- | --- |
| `[Move1]`, `[Move2]` | Main button and sensor bindings for PS Move |
| `[MoveTrigger1]`, `[MoveTrigger2]` | Main trigger settings for PS Move |
| `[MoveGyroscope1]`, `[MoveGyroscope2]` | Main gyroscope settings for PS Move |
| `[MoveAccelerometer1]`, `[MoveAccelerometer2]` | Main accelerometer settings for PS Move |
| `[Rumble]` | PS Move rumble tuning and strength settings |
| `[Navigator1]`, `[Navigator2]` | Main button and stick bindings for PS Navigator |
| `[NavigatorTrigger1]`, `[NavigatorTrigger2]` | Main trigger settings for PS Navigator |
| `[NavigatorStick1]`, `[NavigatorStick2]` | Main stick settings for PS Navigator |
| `[RepeatMs]` | Repeat delay for repeating actions |
| `[Misc]` | Profile metadata |

### Alternate Mode Sections

`Alt*` sections are used while `o:alt_mode` is held.

| Section | Description |
| --- | --- |
| `[AltMove1]`, `[AltMove2]` | Alternate button and sensor bindings for PS Move |
| `[AltMoveTrigger1]`, `[AltMoveTrigger2]` | Alternate trigger settings for PS Move |
| `[AltMoveGyroscope1]`, `[AltMoveGyroscope2]` | Alternate gyroscope settings for PS Move |
| `[AltMoveAccelerometer1]`, `[AltMoveAccelerometer2]` | Alternate accelerometer settings for PS Move |
| `[AltNavigator1]`, `[AltNavigator2]` | Alternate button bindings for PS Navigator |
| `[AltNavigatorTrigger1]`, `[AltNavigatorTrigger2]` | Alternate trigger settings for PS Navigator |
| `[AltNavigatorStick1]`, `[AltNavigatorStick2]` | Alternate stick settings for PS Navigator |

## Section Reference

### PS Move Binding Sections
- `[Move1]`, `[Move2]`
- `[AltMove1]`, `[AltMove2]`

Value format for every binding field:

- `prefix:value`

Multiple actions can be assigned on one line, separated by spaces.

Example:

```ini
CROSS=x:a
MOVE=o:alt_mode
GYRO_Z=m:x
TRIGGER=x:rt
```

Supported fields:

| Name | Description | Value |
| --- | --- | --- |
| `CROSS` | Cross button | `prefix:value` |
| `CIRCLE` | Circle button | `prefix:value` |
| `SQUARE` | Square button | `prefix:value` |
| `TRIANGLE` | Triangle button | `prefix:value` |
| `SELECT` | Select button | `prefix:value` |
| `START` | Start button | `prefix:value` |
| `PS` | PS button | `prefix:value` |
| `MOVE` | Move button | `prefix:value` |
| `TRIGGER` | Trigger button or trigger binding | `prefix:value` |
| `GYRO_X` | Gyroscope X axis | `prefix:value` |
| `GYRO_X+` | Positive gyroscope X axis | `prefix:value` |
| `GYRO_X-` | Negative gyroscope X axis | `prefix:value` |
| `GYRO_Y` | Gyroscope Y axis | `prefix:value` |
| `GYRO_Y+` | Positive gyroscope Y axis | `prefix:value` |
| `GYRO_Y-` | Negative gyroscope Y axis | `prefix:value` |
| `GYRO_Z` | Gyroscope Z axis | `prefix:value` |
| `GYRO_Z+` | Positive gyroscope Z axis | `prefix:value` |
| `GYRO_Z-` | Negative gyroscope Z axis | `prefix:value` |
| `ACCEL_X` | Accelerometer X axis | `prefix:value` |
| `ACCEL_X+` | Positive accelerometer X axis | `prefix:value` |
| `ACCEL_X-` | Negative accelerometer X axis | `prefix:value` |
| `ACCEL_Y` | Accelerometer Y axis | `prefix:value` |
| `ACCEL_Y+` | Positive accelerometer Y axis | `prefix:value` |
| `ACCEL_Y-` | Negative accelerometer Y axis | `prefix:value` |
| `ACCEL_Z` | Accelerometer Z axis | `prefix:value` |
| `ACCEL_Z+` | Positive accelerometer Z axis | `prefix:value` |
| `ACCEL_Z-` | Negative accelerometer Z axis | `prefix:value` |

### PS Move Trigger Sections
- `[MoveTrigger1]`, `[MoveTrigger2]`
- `[AltMoveTrigger1]`, `[AltMoveTrigger2]`

| Name | Description | Value |
| --- | --- | --- |
| `Invert` | Enable trigger inversion | `0` or `1` |
| `Sensitivity` | Trigger input multiplier | `float`, `0.0 - max` |
| `Deadzone` | Ignore trigger input below this threshold | `int`, `0 - 255` |

### PS Move Gyroscope Sections
- `[MoveGyroscope1]`, `[MoveGyroscope2]`
- `[AltMoveGyroscope1]`, `[AltMoveGyroscope2]`

| Name | Description | Value |
| --- | --- | --- |
| `InvertX`, `InvertY`, `InvertZ` | Enable inversion for the axis | `0` or `1` |
| `SensitivityX`, `SensitivityY`, `SensitivityZ` | Axis input multiplier | `float`, `0.0 - max` |
| `DeadzoneX`, `DeadzoneY`, `DeadzoneZ` | Ignore input below this threshold | `int`, `0 - max` |

### PS Move Accelerometer Sections
- `[MoveAccelerometer1]`, `[MoveAccelerometer2]`
- `[AltMoveAccelerometer1]`, `[AltMoveAccelerometer2]`

| Name | Description | Value |
| --- | --- | --- |
| `InvertX`, `InvertY`, `InvertZ` | Enable inversion for the axis | `0` or `1` |
| `SensitivityX`, `SensitivityY`, `SensitivityZ` | Axis input multiplier | `float`, `0.0 - max` |
| `DeadzoneX`, `DeadzoneY`, `DeadzoneZ` | Ignore input below this threshold | `int`, `0 - max` |

### PS Navigator Binding Sections
- `[Navigator1]`, `[Navigator2]`
- `[AltNavigator1]`, `[AltNavigator2]`

Value format for every binding field:

- `prefix:value`

Example:

```ini
CROSS=x:a
L2=x:lt
X=x:lx
Y=x:ly
```

Supported fields:

| Name | Description | Value |
| --- | --- | --- |
| `CROSS` | Cross button | `prefix:value` |
| `CIRCLE` | Circle button | `prefix:value` |
| `L1` | L1 button | `prefix:value` |
| `L2` | L2 button or trigger binding | `prefix:value` |
| `L3` | L3 button | `prefix:value` |
| `PS` | PS button | `prefix:value` |
| `UP` | D-pad up | `prefix:value` |
| `RIGHT` | D-pad right | `prefix:value` |
| `DOWN` | D-pad down | `prefix:value` |
| `LEFT` | D-pad left | `prefix:value` |
| `X` | Stick X axis | `prefix:value` |
| `X+` | Positive stick X axis | `prefix:value` |
| `X-` | Negative stick X axis | `prefix:value` |
| `Y` | Stick Y axis | `prefix:value` |
| `Y+` | Positive stick Y axis | `prefix:value` |
| `Y-` | Negative stick Y axis | `prefix:value` |

### PS Navigator Trigger Sections
- `[NavigatorTrigger1]`, `[NavigatorTrigger2]`
- `[AltNavigatorTrigger1]`, `[AltNavigatorTrigger2]`

| Name | Description | Value |
| --- | --- | --- |
| `Invert` | Enable trigger inversion | `0` or `1` |
| `Sensitivity` | Trigger input multiplier | `float`, `0.0 - max` |
| `Deadzone` | Ignore trigger input below this threshold | `int`, `0 - 255` |

### PS Navigator Stick Sections
- `[NavigatorStick1]`, `[NavigatorStick2]`
- `[AltNavigatorStick1]`, `[AltNavigatorStick2]`

| Name | Description | Value |
| --- | --- | --- |
| `InvertX`, `InvertY` | Enable inversion for the axis | `0` or `1` |
| `SensitivityX`, `SensitivityY` | Axis input multiplier | `float`, `0.0 - max` |
| `DeadzoneX`, `DeadzoneY` | Ignore input below this threshold | `int`, `0 - max` |

### Rumble Section
- `[Rumble]`

| Name | Description | Value |
| --- | --- | --- |
| `Enabled` | Enable rumble output | `0` or `1` |
| `MasterStrength` | Overall rumble strength multiplier | `float`, `0.0 - 4.0` |
| `CombineWhenSingleMove` | Combine motors when only one Move is connected | `0` or `1` |
| `LargeMinActive`, `SmallMinActive` | Minimum source value before a motor becomes active | `int`, `0 - max` |
| `LargeLowThreshold`, `LargeMidThreshold`, `SmallLowThreshold`, `SmallMidThreshold` | Threshold split points for rumble scaling | `int`, `0 - max` |
| `LargeBoostLow`, `LargeBoostMid`, `LargeBoostHigh`, `SmallBoostLow`, `SmallBoostMid`, `SmallBoostHigh` | Strength multipliers for each range | `float`, `0.0 - max` |
| `LargeMinOutput`, `SmallMinOutput` | Minimum motor output once active | `int`, `0 - 255` |

### Repeat Section
- `[RepeatMs]`

| Name | Description | Value |
| --- | --- | --- |
| `Key` | Repeat delay for keyboard/mouse button actions | `int` milliseconds |
| `Wheel` | Repeat delay for mouse wheel actions (default: 0 = use Key) | `int` milliseconds |

### Misc Section
- `[Misc]`

| Name | Description | Value |
| --- | --- | --- |
| `Path` | Optional profile path metadata | Text |
| `Name` | Profile display name | Text |
| `Comments` | Free-form profile notes | Text |
