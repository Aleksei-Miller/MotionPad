#ifndef INPUT_ACTIONS_H
#define INPUT_ACTIONS_H

#include "app_common.h"

typedef enum ActionKind {
    ActionKind_None = 0,
    ActionKind_Keyboard,
    ActionKind_MouseButton,
    ActionKind_MouseWheel,
    ActionKind_MouseWheel_Pos,
    ActionKind_MouseWheel_Neg,
    ActionKind_MouseMoveX,
    ActionKind_MouseMoveX_Pos,
    ActionKind_MouseMoveX_Neg,
    ActionKind_MouseMoveY,
    ActionKind_MouseMoveY_Pos,
    ActionKind_MouseMoveY_Neg,
    ActionKind_XboxButton,
    ActionKind_XboxTrigger,
    ActionKind_XboxAxis,
    ActionKind_XboxAxis_Pos,
    ActionKind_XboxAxis_Neg,
    ActionKind_StopGyroAll,
    ActionKind_StopGyroX,
    ActionKind_StopGyroY,
    ActionKind_StopGyroZ,
    ActionKind_ResetAccelAll,
    ActionKind_ResetAccelX,
    ActionKind_ResetAccelY,
    ActionKind_ResetAccelZ,
    ActionKind_AltMode
} ActionKind;

typedef enum MouseButtonKind {
    MouseButtonKind_None = 0,
    MouseButtonKind_Left,
    MouseButtonKind_Right,
    MouseButtonKind_Middle,
    MouseButtonKind_X1,
    MouseButtonKind_X2
} MouseButtonKind;

typedef enum XboxAxisKind {
    XboxAxisKind_None = 0,
    XboxAxisKind_LT,
    XboxAxisKind_RT,
    XboxAxisKind_LX,
    XboxAxisKind_LY,
    XboxAxisKind_RX,
    XboxAxisKind_RY
} XboxAxisKind;

typedef struct AxisSettings {
    int invert;
    float sensitivity;
    int deadzone;
} AxisSettings;

typedef struct ButtonAction {
    ActionKind kind;
    union {
        UINT vkCode;
        MouseButtonKind mouseButton;
        USHORT xboxButton;
        XboxAxisKind xboxAxis;
        int mouseWheel;
    } data;
    bool isDown;
    DWORD lastRepeatTick;
} ButtonAction;

typedef struct SensorAction {
    ActionKind kind;
    union {
        UINT vkCode;
        MouseButtonKind mouseButton;
        USHORT xboxButton;
        XboxAxisKind xboxAxis;
        int mouseWheel;
    } data;
    bool isDown;
    DWORD lastRepeatTick;
} SensorAction;

UINT resolveVirtualKeyFromName(const wchar_t *key_name);
MouseButtonKind resolveMouseButtonKindFromName(const wchar_t *name);
USHORT resolveXboxButtonFromName(const wchar_t *name);
XboxAxisKind resolveXboxAxisKindFromName(const wchar_t *name);
bool parseBindingToken(const wchar_t *token, ActionKind *out_kind, UINT *out_value);

void sendKeyboardEvent(UINT vk_code, bool is_down);
void sendMouseButtonByKind(MouseButtonKind kind, bool is_down);
void sendMouseMoveDelta(int dx, int dy);
void sendMouseWheelEvent(int wheel_delta);

SHORT clampToShort(int value);
UCHAR clampToByte(int value);
void setXboxAxisValue(XUSB_REPORT *report, XboxAxisKind axis, int value);

void resetButtonActionState(ButtonAction *action);
void resetSensorActionState(SensorAction *action);
int applyAxisDeadzone(int raw_value, const AxisSettings *axis);
int applyTriggerSettings(int raw_value, const AxisSettings *axis);
int normalizeSensorToTrigger(int raw_value);
int normalizeSensorToThumb(int raw_value);
void updateDigitalButtonAction(ButtonAction *action, bool pressed, XUSB_REPORT *report, int trigger_value);
void applySensorButtonLikeAction(SensorAction *action, bool active, DWORD now_tick, int repeat_ms, int wheelRepeatMs, XUSB_REPORT *report);

#endif
