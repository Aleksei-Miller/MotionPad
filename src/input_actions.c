#include "input_actions.h"
#include "profile.h"
#include "logger.h"
#include <math.h>

void sendKeyboardEvent(UINT vk_code, bool is_down)
{
    INPUT input_event;
    ZeroMemory(&input_event, sizeof(input_event));
    input_event.type = INPUT_KEYBOARD;
    input_event.ki.wScan = (WORD)MapVirtualKeyW(vk_code, 0);
    input_event.ki.dwFlags = KEYEVENTF_SCANCODE;
    if (!is_down) input_event.ki.dwFlags |= KEYEVENTF_KEYUP;
    if ((vk_code >= 0x21 && vk_code <= 0x2E) || vk_code == VK_RCONTROL || vk_code == VK_RMENU || vk_code == VK_INSERT || vk_code == VK_DELETE || vk_code == VK_HOME || vk_code == VK_END || vk_code == VK_PRIOR || vk_code == VK_NEXT || vk_code == VK_UP || vk_code == VK_DOWN || vk_code == VK_LEFT || vk_code == VK_RIGHT || vk_code == VK_NUMLOCK || vk_code == VK_SNAPSHOT) {
        input_event.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    SendInput(1, &input_event, sizeof(input_event));
}

static void sendMouseButtonEvent(DWORD down_flag, DWORD up_flag, DWORD mouse_data, bool is_down)
{
    INPUT input_event;
    ZeroMemory(&input_event, sizeof(input_event));
    input_event.type = INPUT_MOUSE;
    input_event.mi.dwFlags = is_down ? down_flag : up_flag;
    input_event.mi.mouseData = mouse_data;
    SendInput(1, &input_event, sizeof(input_event));
}

void sendMouseWheelEvent(int wheel_delta)
{
    INPUT input_event;
    ZeroMemory(&input_event, sizeof(input_event));
    input_event.type = INPUT_MOUSE;
    input_event.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input_event.mi.mouseData = (DWORD)wheel_delta;
    SendInput(1, &input_event, sizeof(input_event));
}

void sendMouseMoveDelta(int dx, int dy)
{
    INPUT input_event;
    if (dx == 0 && dy == 0) return;
    ZeroMemory(&input_event, sizeof(input_event));
    input_event.type = INPUT_MOUSE;
    input_event.mi.dwFlags = MOUSEEVENTF_MOVE;
    input_event.mi.dx = dx;
    input_event.mi.dy = dy;
    SendInput(1, &input_event, sizeof(input_event));
}

UINT resolveVirtualKeyFromName(const wchar_t *key_name)
{
    size_t name_len;
    if (!key_name || key_name[0] == L'\0') return 0;
    name_len = wcslen(key_name);
    if (name_len == 1) {
        wchar_t ch = towupper(key_name[0]);
        if ((ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9')) return (UINT)ch;
    }
    if ((key_name[0] == L'F' || key_name[0] == L'f') && name_len >= 2 && name_len <= 3) {
        int fn_index = _wtoi(key_name + 1);
        if (fn_index >= 1 && fn_index <= 24) return (UINT)(VK_F1 + (fn_index - 1));
    }
#define VKCMP(a,b) if (_wcsicmp(key_name, L##a) == 0) return b
    VKCMP("ENTER", VK_RETURN); VKCMP("RETURN", VK_RETURN); VKCMP("SPACE", VK_SPACE); VKCMP("TAB", VK_TAB);
    VKCMP("ESC", VK_ESCAPE); VKCMP("ESCAPE", VK_ESCAPE); VKCMP("BACKSPACE", VK_BACK); VKCMP("BACK", VK_BACK);
    VKCMP("SHIFT", VK_SHIFT); VKCMP("LSHIFT", VK_LSHIFT); VKCMP("RSHIFT", VK_RSHIFT);
    VKCMP("CTRL", VK_CONTROL); VKCMP("CONTROL", VK_CONTROL); VKCMP("LCTRL", VK_LCONTROL); VKCMP("LCONTROL", VK_LCONTROL); VKCMP("RCTRL", VK_RCONTROL); VKCMP("RCONTROL", VK_RCONTROL);
    VKCMP("ALT", VK_MENU); VKCMP("LALT", VK_LMENU); VKCMP("RALT", VK_RMENU);
    VKCMP("UP", VK_UP); VKCMP("DOWN", VK_DOWN); VKCMP("LEFT", VK_LEFT); VKCMP("RIGHT", VK_RIGHT);
    VKCMP("HOME", VK_HOME); VKCMP("END", VK_END); VKCMP("PGUP", VK_PRIOR); VKCMP("PAGEUP", VK_PRIOR); VKCMP("PGDN", VK_NEXT); VKCMP("PAGEDOWN", VK_NEXT);
    VKCMP("INSERT", VK_INSERT); VKCMP("INS", VK_INSERT); VKCMP("DELETE", VK_DELETE); VKCMP("DEL", VK_DELETE);
    VKCMP("CAPSLOCK", VK_CAPITAL); VKCMP("WIN", VK_LWIN); VKCMP("LWIN", VK_LWIN); VKCMP("RWIN", VK_RWIN);
    VKCMP("APPS", VK_APPS); VKCMP("MENU", VK_APPS); VKCMP("PRINTSCREEN", VK_SNAPSHOT); VKCMP("PRTSC", VK_SNAPSHOT); VKCMP("SCROLLLOCK", VK_SCROLL); VKCMP("PAUSE", VK_PAUSE); VKCMP("NUMLOCK", VK_NUMLOCK);
    VKCMP("NUMPAD0", VK_NUMPAD0); VKCMP("NUMPAD1", VK_NUMPAD1); VKCMP("NUMPAD2", VK_NUMPAD2); VKCMP("NUMPAD3", VK_NUMPAD3); VKCMP("NUMPAD4", VK_NUMPAD4); VKCMP("NUMPAD5", VK_NUMPAD5); VKCMP("NUMPAD6", VK_NUMPAD6); VKCMP("NUMPAD7", VK_NUMPAD7); VKCMP("NUMPAD8", VK_NUMPAD8); VKCMP("NUMPAD9", VK_NUMPAD9);
    VKCMP("ADD", VK_ADD); VKCMP("SUBTRACT", VK_SUBTRACT); VKCMP("MULTIPLY", VK_MULTIPLY); VKCMP("DIVIDE", VK_DIVIDE); VKCMP("DECIMAL", VK_DECIMAL);
#undef VKCMP
    return 0;
}

MouseButtonKind resolveMouseButtonKindFromName(const wchar_t *name)
{
    if (!name || name[0] == L'\0') return MouseButtonKind_None;
    if (_wcsicmp(name, L"LB") == 0 || _wcsicmp(name, L"LEFT") == 0) return MouseButtonKind_Left;
    if (_wcsicmp(name, L"RB") == 0 || _wcsicmp(name, L"RIGHT") == 0) return MouseButtonKind_Right;
    if (_wcsicmp(name, L"MB") == 0 || _wcsicmp(name, L"MIDDLE") == 0) return MouseButtonKind_Middle;
    if (_wcsicmp(name, L"X1") == 0) return MouseButtonKind_X1;
    if (_wcsicmp(name, L"X2") == 0) return MouseButtonKind_X2;
    return MouseButtonKind_None;
}

void sendMouseButtonByKind(MouseButtonKind kind, bool is_down)
{
    switch (kind) {
        case MouseButtonKind_Left: sendMouseButtonEvent(MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, 0, is_down); break;
        case MouseButtonKind_Right: sendMouseButtonEvent(MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP, 0, is_down); break;
        case MouseButtonKind_Middle: sendMouseButtonEvent(MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP, 0, is_down); break;
        case MouseButtonKind_X1: sendMouseButtonEvent(MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON1, is_down); break;
        case MouseButtonKind_X2: sendMouseButtonEvent(MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON2, is_down); break;
        default: break;
    }
}

USHORT resolveXboxButtonFromName(const wchar_t *name)
{
    if (!name || name[0] == L'\0') return 0;
#define XBCMP(a,b) if (_wcsicmp(name, L##a) == 0) return b
    XBCMP("A", XUSB_GAMEPAD_A); XBCMP("B", XUSB_GAMEPAD_B); XBCMP("X", XUSB_GAMEPAD_X); XBCMP("Y", XUSB_GAMEPAD_Y);
    XBCMP("LB", XUSB_GAMEPAD_LEFT_SHOULDER); XBCMP("RB", XUSB_GAMEPAD_RIGHT_SHOULDER); XBCMP("BACK", XUSB_GAMEPAD_BACK); XBCMP("START", XUSB_GAMEPAD_START); XBCMP("LS", XUSB_GAMEPAD_LEFT_THUMB); XBCMP("RS", XUSB_GAMEPAD_RIGHT_THUMB); XBCMP("GUIDE", XUSB_GAMEPAD_GUIDE); XBCMP("UP", XUSB_GAMEPAD_DPAD_UP); XBCMP("DOWN", XUSB_GAMEPAD_DPAD_DOWN); XBCMP("LEFT", XUSB_GAMEPAD_DPAD_LEFT); XBCMP("RIGHT", XUSB_GAMEPAD_DPAD_RIGHT);
#undef XBCMP
    return 0;
}

XboxAxisKind resolveXboxAxisKindFromName(const wchar_t *name)
{
    if (!name || name[0] == L'\0') return XboxAxisKind_None;
    if (_wcsicmp(name, L"LT") == 0) return XboxAxisKind_LT;
    if (_wcsicmp(name, L"RT") == 0) return XboxAxisKind_RT;
    if (_wcsicmp(name, L"LX") == 0) return XboxAxisKind_LX;
    if (_wcsicmp(name, L"LY") == 0) return XboxAxisKind_LY;
    if (_wcsicmp(name, L"RX") == 0) return XboxAxisKind_RX;
    if (_wcsicmp(name, L"RY") == 0) return XboxAxisKind_RY;
    return XboxAxisKind_None;
}

bool parseBindingToken(const wchar_t *token, ActionKind *out_kind, UINT *out_value)
{
    wchar_t buffer[64], *colon_ptr, *prefix, *name;
    if (!token || !out_kind || !out_value) return false;
    ZeroMemory(buffer, sizeof(buffer));
    wcsncpy(buffer, token, 63);
    trimWideStringInPlace(buffer);
    if (buffer[0] == L'\0') return false;
    colon_ptr = wcschr(buffer, L':');
    if (!colon_ptr) return false;
    *colon_ptr = L'\0';
    prefix = buffer;
    name = colon_ptr + 1;
    trimWideStringInPlace(prefix); trimWideStringInPlace(name);
    if (_wcsicmp(prefix, L"k") == 0) {
        UINT vk = resolveVirtualKeyFromName(name); if (!vk) return false; *out_kind = ActionKind_Keyboard; *out_value = vk; return true;
    }
    if (_wcsicmp(prefix, L"m") == 0) {
        MouseButtonKind mb;
        if (_wcsicmp(name, L"x") == 0) { *out_kind = ActionKind_MouseMoveX; *out_value = 0; return true; }
        if (_wcsicmp(name, L"x+") == 0) { *out_kind = ActionKind_MouseMoveX_Pos; *out_value = 0; return true; }
        if (_wcsicmp(name, L"x-") == 0) { *out_kind = ActionKind_MouseMoveX_Neg; *out_value = 0; return true; }
        if (_wcsicmp(name, L"y") == 0) { *out_kind = ActionKind_MouseMoveY; *out_value = 0; return true; }
        if (_wcsicmp(name, L"y+") == 0) { *out_kind = ActionKind_MouseMoveY_Pos; *out_value = 0; return true; }
        if (_wcsicmp(name, L"y-") == 0) { *out_kind = ActionKind_MouseMoveY_Neg; *out_value = 0; return true; }
        if (_wcsicmp(name, L"mw") == 0) { *out_kind = ActionKind_MouseWheel; *out_value = 0; return true; }
        if (_wcsicmp(name, L"mw+") == 0) { *out_kind = ActionKind_MouseWheel_Pos; *out_value = 0; return true; }
        if (_wcsicmp(name, L"mw-") == 0) { *out_kind = ActionKind_MouseWheel_Neg; *out_value = 0; return true; }
        mb = resolveMouseButtonKindFromName(name); if (mb == MouseButtonKind_None) return false; *out_kind = ActionKind_MouseButton; *out_value = (UINT)mb; return true;
    }
    if (_wcsicmp(prefix, L"x") == 0) {
        USHORT xb = resolveXboxButtonFromName(name); XboxAxisKind axis = resolveXboxAxisKindFromName(name);
        if (xb != 0) { *out_kind = ActionKind_XboxButton; *out_value = xb; return true; }
        if (_wcsicmp(name, L"lx+") == 0) { *out_kind = ActionKind_XboxAxis_Pos; *out_value = (UINT)XboxAxisKind_LX; return true; }
        if (_wcsicmp(name, L"lx-") == 0) { *out_kind = ActionKind_XboxAxis_Neg; *out_value = (UINT)XboxAxisKind_LX; return true; }
        if (_wcsicmp(name, L"ly+") == 0) { *out_kind = ActionKind_XboxAxis_Pos; *out_value = (UINT)XboxAxisKind_LY; return true; }
        if (_wcsicmp(name, L"ly-") == 0) { *out_kind = ActionKind_XboxAxis_Neg; *out_value = (UINT)XboxAxisKind_LY; return true; }
        if (_wcsicmp(name, L"rx+") == 0) { *out_kind = ActionKind_XboxAxis_Pos; *out_value = (UINT)XboxAxisKind_RX; return true; }
        if (_wcsicmp(name, L"rx-") == 0) { *out_kind = ActionKind_XboxAxis_Neg; *out_value = (UINT)XboxAxisKind_RX; return true; }
        if (_wcsicmp(name, L"ry+") == 0) { *out_kind = ActionKind_XboxAxis_Pos; *out_value = (UINT)XboxAxisKind_RY; return true; }
        if (_wcsicmp(name, L"ry-") == 0) { *out_kind = ActionKind_XboxAxis_Neg; *out_value = (UINT)XboxAxisKind_RY; return true; }
        if (axis != XboxAxisKind_None) { *out_kind = (axis == XboxAxisKind_LT || axis == XboxAxisKind_RT) ? ActionKind_XboxTrigger : ActionKind_XboxAxis; *out_value = (UINT)axis; return true; }
        return false;
    }
    if (_wcsicmp(prefix, L"o") == 0) {
#define OTH(nameTxt, kindVal) if (_wcsicmp(name, L##nameTxt) == 0) { *out_kind = kindVal; *out_value = 0; return true; }
        OTH("gyro_stop_all", ActionKind_StopGyroAll); OTH("gyro_stop_x", ActionKind_StopGyroX); OTH("gyro_stop_y", ActionKind_StopGyroY); OTH("gyro_stop_z", ActionKind_StopGyroZ);
        OTH("accel_reset_all", ActionKind_ResetAccelAll); OTH("accel_reset_x", ActionKind_ResetAccelX); OTH("accel_reset_y", ActionKind_ResetAccelY); OTH("accel_reset_z", ActionKind_ResetAccelZ);
        OTH("alt_mode", ActionKind_AltMode);
#undef OTH
        return false;
    }
    return false;
}

SHORT clampToShort(int value) { if (value < -32768) return -32768; if (value > 32767) return 32767; return (SHORT)value; }
UCHAR clampToByte(int value) { if (value < 0) return 0; if (value > 255) return 255; return (UCHAR)value; }

void setXboxAxisValue(XUSB_REPORT *report, XboxAxisKind axis, int value)
{
    if (!report) return;
    switch (axis) {
        case XboxAxisKind_LT: report->bLeftTrigger = (BYTE)value; break;
        case XboxAxisKind_RT: report->bRightTrigger = (BYTE)value; break;
        case XboxAxisKind_LX: report->sThumbLX = (SHORT)value; break;
        case XboxAxisKind_LY: report->sThumbLY = (SHORT)value; break;
        case XboxAxisKind_RX: report->sThumbRX = (SHORT)value; break;
        case XboxAxisKind_RY: report->sThumbRY = (SHORT)value; break;
        default: break;
    }
}

void resetButtonActionState(ButtonAction *action)
{
    if (!action) return;
    if (action->isDown) {
        if (action->kind == ActionKind_Keyboard) sendKeyboardEvent(action->data.vkCode, false);
        else if (action->kind == ActionKind_MouseButton) sendMouseButtonByKind(action->data.mouseButton, false);
    }
    action->isDown = false; action->lastRepeatTick = 0;
}

void resetSensorActionState(SensorAction *action)
{
    if (!action) return;
    if (action->isDown) {
        if (action->kind == ActionKind_Keyboard) sendKeyboardEvent(action->data.vkCode, false);
        else if (action->kind == ActionKind_MouseButton) sendMouseButtonByKind(action->data.mouseButton, false);
    }
    action->isDown = false; action->lastRepeatTick = 0;
}

int applyAxisDeadzone(int raw_value, const AxisSettings *axis)
{
    int magnitude, adjusted;
    if (!axis) return raw_value;
    if (axis->invert) raw_value = -raw_value;
    if (axis->sensitivity != 1.0f) {
        raw_value = (int)roundf((float)raw_value * axis->sensitivity);
    }
    magnitude = abs(raw_value);
    if (magnitude <= axis->deadzone) return 0;
    adjusted = magnitude - axis->deadzone;
    return raw_value < 0 ? -adjusted : adjusted;
}

int applyStickAxis(int raw_value, const AxisSettings *axis)
{
    int magnitude, adjusted;
    if (!axis) return raw_value;
    if (axis->invert) raw_value = -raw_value;
    if (axis->sensitivity > 0.0f && axis->sensitivity != 1.0f && raw_value != 0) {
        float sign = raw_value > 0 ? 1.0f : -1.0f;
        float max_val = raw_value > 0 ? 127.0f : 128.0f;
        float norm = (float)abs(raw_value) / max_val;
        if (norm > 1.0f) norm = 1.0f;
        float curved = (float)pow((double)norm, 1.0 / (double)axis->sensitivity);
        raw_value = (int)(sign * curved * 127.0f);
    } else if (axis->sensitivity <= 0.0f) {
        return 0;
    }
    magnitude = abs(raw_value);
    if (magnitude <= axis->deadzone) return 0;
    adjusted = magnitude - axis->deadzone;
    return raw_value < 0 ? -adjusted : adjusted;
}

int applyTriggerSettings(int raw_value, const AxisSettings *axis)
{
    if (!axis) {
        return clampToByte(raw_value);
    }

    raw_value = clampToByte(raw_value);
    if (axis->invert) {
        raw_value = 255 - raw_value;
    }

    if (axis->sensitivity > 0.0f && axis->sensitivity != 1.0f) {
        float norm = (float)raw_value / 255.0f;
        float curved = (float)pow((double)norm, 1.0 / axis->sensitivity);
        raw_value = (int)(curved * 255.0f);
    } else if (axis->sensitivity <= 0.0f) {
        return 0;
    }
    raw_value = clampToByte(raw_value);
    if (raw_value <= axis->deadzone) {
        return 0;
    }

    return clampToByte(raw_value - axis->deadzone);
}

int normalizeSensorToTrigger(int raw_value)
{
    int magnitude = abs(raw_value);
    if (magnitude > 255) magnitude = 255;
    return magnitude;
}

int normalizeSensorToThumb(int raw_value)
{
    return clampToShort(raw_value * 16);
}

void updateDigitalButtonAction(ButtonAction *action, bool pressed, XUSB_REPORT *report, int trigger_value, int repeatMs, int mouseRepeatMs, int wheelRepeatMs, DWORD now)
{
    if (!action) return;
    switch (action->kind) {
        case ActionKind_Keyboard:
            if (pressed) {
                if (!action->isDown) {
                    sendKeyboardEvent(action->data.vkCode, true);
                    action->isDown = true;
                    action->lastRepeatTick = now;
                } else if (repeatMs > 0 && now - action->lastRepeatTick >= (DWORD)repeatMs) {
                    sendKeyboardEvent(action->data.vkCode, false);
                    sendKeyboardEvent(action->data.vkCode, true);
                    action->lastRepeatTick = now;
                }
            } else {
                if (action->isDown) {
                    sendKeyboardEvent(action->data.vkCode, false);
                    action->isDown = false;
                }
            }
            break;
        case ActionKind_MouseButton:
            if (pressed) {
                if (!action->isDown) {
                    sendMouseButtonByKind(action->data.mouseButton, true);
                    action->isDown = true;
                    action->lastRepeatTick = now;
                } else if (mouseRepeatMs > 0 && now - action->lastRepeatTick >= (DWORD)mouseRepeatMs) {
                    sendMouseButtonByKind(action->data.mouseButton, false);
                    sendMouseButtonByKind(action->data.mouseButton, true);
                    action->lastRepeatTick = now;
                }
            } else {
                if (action->isDown) {
                    sendMouseButtonByKind(action->data.mouseButton, false);
                    action->isDown = false;
                }
            }
            break;
        case ActionKind_MouseWheel:
        case ActionKind_MouseWheel_Pos:
        case ActionKind_MouseWheel_Neg: {
            int wheel_delta;
            int effective_repeat = wheelRepeatMs > 0 ? wheelRepeatMs : 50;
            if (action->kind == ActionKind_MouseWheel_Neg) wheel_delta = -WHEEL_DELTA;
            else wheel_delta = action->data.mouseWheel != 0 ? action->data.mouseWheel * WHEEL_DELTA : WHEEL_DELTA;
            if (pressed && !action->isDown) {
                sendMouseWheelEvent(wheel_delta);
                action->isDown = true;
                action->lastRepeatTick = now;
            } else if (pressed && action->isDown) {
                if (now - action->lastRepeatTick >= (DWORD)effective_repeat) {
                    sendMouseWheelEvent(wheel_delta);
                    action->lastRepeatTick = now;
                }
            } else {
                action->isDown = false;
            }
            break;
        }
        case ActionKind_XboxButton:
            if (pressed) report->wButtons |= action->data.xboxButton;
            break;
        case ActionKind_XboxTrigger:
            if (pressed) {
                if (action->data.xboxAxis == XboxAxisKind_LT) { if ((BYTE)trigger_value > report->bLeftTrigger) report->bLeftTrigger = (BYTE)trigger_value; }
                else if (action->data.xboxAxis == XboxAxisKind_RT) { if ((BYTE)trigger_value > report->bRightTrigger) report->bRightTrigger = (BYTE)trigger_value; }
            }
            break;
        default: break;
    }
}

void applySensorButtonLikeAction(SensorAction *action, bool active, DWORD now_tick, int repeat_ms, int mouseRepeatMs, int wheelRepeatMs, XUSB_REPORT *report)
{
    int effectiveRepeat = repeat_ms;
    if (action->kind == ActionKind_MouseWheel && wheelRepeatMs > 0) {
        effectiveRepeat = wheelRepeatMs;
    }
    if (!action || !report) return;
    if (!active) {
        if (action->isDown) {
            if (action->kind == ActionKind_Keyboard) sendKeyboardEvent(action->data.vkCode, false);
            else if (action->kind == ActionKind_MouseButton) sendMouseButtonByKind(action->data.mouseButton, false);
            else if (action->kind == ActionKind_XboxButton) report->wButtons &= ~action->data.xboxButton;
        }
        action->isDown = false;
        action->lastRepeatTick = 0;
        return;
    }
    if (action->kind == ActionKind_XboxButton) { action->isDown = true; report->wButtons |= action->data.xboxButton; return; }
    if (action->kind == ActionKind_MouseWheel || action->kind == ActionKind_MouseWheel_Pos || action->kind == ActionKind_MouseWheel_Neg) {
        if (action->lastRepeatTick == 0 || now_tick - action->lastRepeatTick >= (DWORD)effectiveRepeat) {
            sendMouseWheelEvent(action->data.mouseWheel * WHEEL_DELTA);
            action->lastRepeatTick = now_tick;
        }
        return;
    }
    if (!action->isDown) {
        if (action->kind == ActionKind_Keyboard) sendKeyboardEvent(action->data.vkCode, true);
        else if (action->kind == ActionKind_MouseButton) sendMouseButtonByKind(action->data.mouseButton, true);
        action->isDown = true;
        action->lastRepeatTick = now_tick;
        return;
    }
    if (action->kind == ActionKind_Keyboard) {
        if (action->lastRepeatTick == 0 || now_tick - action->lastRepeatTick >= (DWORD)repeat_ms) {
            sendKeyboardEvent(action->data.vkCode, false);
            sendKeyboardEvent(action->data.vkCode, true);
            action->lastRepeatTick = now_tick;
        }
    } else if (action->kind == ActionKind_MouseButton) {
        if (action->lastRepeatTick == 0 || now_tick - action->lastRepeatTick >= (DWORD)mouseRepeatMs) {
            sendMouseButtonByKind(action->data.mouseButton, false);
            sendMouseButtonByKind(action->data.mouseButton, true);
            action->lastRepeatTick = now_tick;
        }
    }
}
