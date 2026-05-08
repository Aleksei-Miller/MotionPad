#include "navigator_input.h"
#include "logger.h"

void navigatorProfileResetRuntimeState(NavigatorProfile *profile)
{
    int i, j;
    if (!profile) return;
    for (i = 0; i < profile->buttonBindingCount; ++i) {
        for (j = 0; j < profile->buttonBindings[i].actionCount; ++j) {
            resetButtonActionState(&profile->buttonBindings[i].actions[j]);
        }
    }
    for (i = 0; i < profile->altButtonBindingCount; ++i) {
        for (j = 0; j < profile->altButtonBindings[i].actionCount; ++j) {
            resetButtonActionState(&profile->altButtonBindings[i].actions[j]);
        }
    }
    for (i = 0; i < NavigatorAxisInput_Count; ++i) {
        for (j = 0; j < profile->sensorBindings[i].actionCount; ++j) {
            resetSensorActionState(&profile->sensorBindings[i].actions[j]);
        }
        for (j = 0; j < profile->altSensorBindings[i].actionCount; ++j) {
            resetSensorActionState(&profile->altSensorBindings[i].actions[j]);
        }
    }
    profile->altModeActive = false;
    profile->mouseResidualX = 0.0f;
    profile->mouseResidualY = 0.0f;
}

static int adjustDirectionalAxisValue(int processed_value, NavigatorAxisInput input)
{
    switch (input) {
        case NavigatorAxisInput_X_Pos:
        case NavigatorAxisInput_Y_Pos:
            return processed_value > 0 ? processed_value : 0;
        case NavigatorAxisInput_X_Neg:
        case NavigatorAxisInput_Y_Neg:
            return processed_value < 0 ? -processed_value : 0;
        default:
            return processed_value;
    }
}

static SHORT normalizeNavigatorStickToThumb(int raw_value)
{
    return clampToShort(raw_value * 256);
}

static void applyNavigatorMouseDelta(NavigatorProfile *profile, float delta_x, float delta_y)
{
    int dx, dy;
    profile->mouseResidualX += delta_x;
    profile->mouseResidualY += delta_y;
    dx = (int)profile->mouseResidualX;
    dy = (int)profile->mouseResidualY;
    profile->mouseResidualX -= (float)dx;
    profile->mouseResidualY -= (float)dy;
    sendMouseMoveDelta(dx, dy);
}

static void applyNavigatorSensorBinding(NavigatorProfile *profile, NavigatorSensorBinding *binding, int processed_value, const AxisSettings *axis, NavigatorAxisInput input, int repeat_ms, int wheelRepeatMs, XUSB_REPORT *report)
{
    DWORD now_tick = GetTickCount();
    int j;
    float mouse_dx = 0.0f, mouse_dy = 0.0f;
    int magnitude;
    (void)axis;

    processed_value = adjustDirectionalAxisValue(processed_value, input);
    magnitude = abs(processed_value);
    for (j = 0; j < binding->actionCount; ++j) {
        SensorAction *action = &binding->actions[j];
        bool active = (processed_value != 0);
        switch (action->kind) {
            case ActionKind_MouseMoveX: if (processed_value != 0) mouse_dx += (float)processed_value; break;
            case ActionKind_MouseMoveX_Pos: if (magnitude != 0) mouse_dx += (float)magnitude; break;
            case ActionKind_MouseMoveX_Neg: if (magnitude != 0) mouse_dx -= (float)magnitude; break;
            case ActionKind_MouseMoveY: if (processed_value != 0) mouse_dy += (float)processed_value; break;
            case ActionKind_MouseMoveY_Pos: if (magnitude != 0) mouse_dy += (float)magnitude; break;
            case ActionKind_MouseMoveY_Neg: if (magnitude != 0) mouse_dy -= (float)magnitude; break;
            case ActionKind_XboxAxis: setXboxAxisValue(report, action->data.xboxAxis, normalizeNavigatorStickToThumb(processed_value)); break;
            case ActionKind_XboxAxis_Pos: if (magnitude != 0) setXboxAxisValue(report, action->data.xboxAxis, normalizeNavigatorStickToThumb(magnitude)); break;
            case ActionKind_XboxAxis_Neg: if (magnitude != 0) setXboxAxisValue(report, action->data.xboxAxis, normalizeNavigatorStickToThumb(-magnitude)); break;
            case ActionKind_XboxTrigger: setXboxAxisValue(report, action->data.xboxAxis, normalizeSensorToTrigger(processed_value)); break;
            case ActionKind_MouseWheel: if (active && (now_tick - action->lastRepeatTick >= (DWORD)wheelRepeatMs || action->lastRepeatTick == 0)) { sendMouseWheelEvent(processed_value > 0 ? WHEEL_DELTA : -WHEEL_DELTA); action->lastRepeatTick = now_tick; } break;
            case ActionKind_MouseWheel_Pos: if (magnitude != 0) sendMouseWheelEvent(WHEEL_DELTA); break;
            case ActionKind_MouseWheel_Neg: if (magnitude != 0) sendMouseWheelEvent(-WHEEL_DELTA); break;
            case ActionKind_Keyboard:
            case ActionKind_MouseButton:
            case ActionKind_XboxButton:
                applySensorButtonLikeAction(action, active, now_tick, repeat_ms, wheelRepeatMs, report);
                break;
            default:
                break;
        }
    }
    if (mouse_dx != 0.0f || mouse_dy != 0.0f) {
        applyNavigatorMouseDelta(profile, mouse_dx, mouse_dy);
    }
}

static bool isNavigatorButtonPressed(PSNavigator *nav, NavigatorButtonInput input)
{
    switch (input) {
        case NavigatorButtonInput_Cross: return psnavigatorGetButton(nav, PSNAV_BUTTON_CROSS) ? true : false;
        case NavigatorButtonInput_Circle: return psnavigatorGetButton(nav, PSNAV_BUTTON_CIRCLE) ? true : false;
        case NavigatorButtonInput_L1: return psnavigatorGetButton(nav, PSNAV_BUTTON_L1) ? true : false;
        case NavigatorButtonInput_L2: return psnavigatorGetButton(nav, PSNAV_BUTTON_L2) ? true : false;
        case NavigatorButtonInput_L3: return psnavigatorGetButton(nav, PSNAV_BUTTON_L3) ? true : false;
        case NavigatorButtonInput_PS: return psnavigatorGetButton(nav, PSNAV_BUTTON_PS) ? true : false;
        case NavigatorButtonInput_Up: return psnavigatorGetButton(nav, PSNAV_BUTTON_UP) ? true : false;
        case NavigatorButtonInput_Right: return psnavigatorGetButton(nav, PSNAV_BUTTON_RIGHT) ? true : false;
        case NavigatorButtonInput_Down: return psnavigatorGetButton(nav, PSNAV_BUTTON_DOWN) ? true : false;
        case NavigatorButtonInput_Left: return psnavigatorGetButton(nav, PSNAV_BUTTON_LEFT) ? true : false;
        default: return false;
    }
}

static void applyNavigatorButtonBindingArray(PSNavigator *nav, NavigatorButtonBinding *bindings, int binding_count, int trigger_value, XUSB_REPORT *report, bool *alt_mode_requested)
{
    int i;
    int j;

    for (i = 0; i < binding_count; ++i) {
        NavigatorButtonBinding *binding = &bindings[i];
        bool pressed = (binding->input == NavigatorButtonInput_L2) ? (trigger_value > 0) : isNavigatorButtonPressed(nav, binding->input);

        for (j = 0; j < binding->actionCount; ++j) {
            ButtonAction *action = &binding->actions[j];

            if (action->kind == ActionKind_AltMode) {
                if (pressed && alt_mode_requested) {
                    *alt_mode_requested = true;
                }
                continue;
            }

            if (binding->input == NavigatorButtonInput_L2 && action->kind == ActionKind_XboxTrigger) {
                setXboxAxisValue(report, action->data.xboxAxis, trigger_value);
            } else {
                updateDigitalButtonAction(action, pressed, report, trigger_value);
            }
        }
    }
}

void processNavigatorInputDevice(PSNavigator *nav, NavigatorProfile *profile, XUSB_REPORT *report)
{
    PSNavResult poll_result;
    int sx;
    int sy;
    int px;
    int py;
    int i;
    int j;
    int repeat_ms;
    int raw_trigger_value;
    int trigger_value;
    bool alt_mode_requested = false;
    NavigatorSensorBinding *active_sensor_bindings;
    AxisSettings *active_stick_config;
    AxisSettings *active_trigger_config;

    if (!nav || !profile || !report || !psnavigatorIsConnected(nav)) {
        return;
    }

    poll_result = psnavigatorPoll(nav);
    if (poll_result != PSNAV_RESULT_OK) {
        logWrite(
            "navigator",
            "poll failed: %d (%s): %s",
            (int)poll_result,
            psnavigatorResultToString(poll_result),
            psnavigatorGetLastError(nav)
        );
        return;
    }

    raw_trigger_value = psnavigatorGetAxis(nav, PSNAV_AXIS_TRIGGER);
    trigger_value = applyTriggerSettings(raw_trigger_value, &profile->triggerConfig);

    applyNavigatorButtonBindingArray(nav, profile->buttonBindings, profile->buttonBindingCount, trigger_value, report, &alt_mode_requested);

    repeat_ms = profile->repeatMs;
    sx = psnavigatorGetAxis(nav, PSNAV_AXIS_STICK_X);
    sy = psnavigatorGetAxis(nav, PSNAV_AXIS_STICK_Y);

    if (profile->altModeActive != alt_mode_requested) {
        for (i = 0; i < NavigatorAxisInput_Count; ++i) {
            for (j = 0; j < profile->sensorBindings[i].actionCount; ++j) {
                resetSensorActionState(&profile->sensorBindings[i].actions[j]);
            }
            for (j = 0; j < profile->altSensorBindings[i].actionCount; ++j) {
                resetSensorActionState(&profile->altSensorBindings[i].actions[j]);
            }
        }
        profile->mouseResidualX = 0.0f;
        profile->mouseResidualY = 0.0f;
        profile->altModeActive = alt_mode_requested;
    }

    if (alt_mode_requested) {
        trigger_value = applyTriggerSettings(raw_trigger_value, &profile->altTriggerConfig);
        applyNavigatorButtonBindingArray(nav, profile->altButtonBindings, profile->altButtonBindingCount, trigger_value, report, NULL);
    }

    active_sensor_bindings = alt_mode_requested ? profile->altSensorBindings : profile->sensorBindings;
    active_trigger_config = alt_mode_requested ? &profile->altTriggerConfig : &profile->triggerConfig;
    trigger_value = applyTriggerSettings(raw_trigger_value, active_trigger_config);
    active_stick_config = alt_mode_requested ? profile->altStickConfig : profile->stickConfig;
    px = applyAxisDeadzone(sx, &active_stick_config[0]);
    py = applyAxisDeadzone(sy, &active_stick_config[1]);

#define NAP(inputName,val,cfg) applyNavigatorSensorBinding(profile, &active_sensor_bindings[inputName], val, cfg, inputName, repeat_ms, profile->wheelRepeatMs, report)
    NAP(NavigatorAxisInput_X, px, &active_stick_config[0]);
    NAP(NavigatorAxisInput_X_Pos, px, &active_stick_config[0]);
    NAP(NavigatorAxisInput_X_Neg, px, &active_stick_config[0]);
    NAP(NavigatorAxisInput_Y, py, &active_stick_config[1]);
    NAP(NavigatorAxisInput_Y_Pos, py, &active_stick_config[1]);
    NAP(NavigatorAxisInput_Y_Neg, py, &active_stick_config[1]);
#undef NAP
}
