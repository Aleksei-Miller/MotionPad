#include "move_input.h"

static void resetMoveSensorBindingArrayState(SensorBinding bindings[SensorAxisInput_Count])
{
    int i, j;
    for (i = 0; i < SensorAxisInput_Count; ++i) {
        for (j = 0; j < bindings[i].actionCount; ++j) {
            resetSensorActionState(&bindings[i].actions[j]);
        }
    }
}

static unsigned int resolvePsMoveButtonMask(MoveButtonInput input)
{
    switch (input) {
        case MoveButtonInput_Cross: return Btn_CROSS;
        case MoveButtonInput_Circle: return Btn_CIRCLE;
        case MoveButtonInput_Square: return Btn_SQUARE;
        case MoveButtonInput_Triangle: return Btn_TRIANGLE;
        case MoveButtonInput_Select: return Btn_SELECT;
        case MoveButtonInput_Start: return Btn_START;
        case MoveButtonInput_PS: return Btn_PS;
        case MoveButtonInput_Move: return Btn_MOVE;
        case MoveButtonInput_Trigger: return Btn_T;
        default: return 0;
    }
}

void moveProfileResetRuntimeState(MoveProfile *profile)
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
    resetMoveSensorBindingArrayState(profile->sensorBindings);
    resetMoveSensorBindingArrayState(profile->altSensorBindings);
    for (i = 0; i < SensorAxisIndex_Count; ++i) {
        profile->accelCenter[i] = 0;
        profile->accelCenterCaptured[i] = false;
        profile->accelResetLatched[i] = false;
    }
    profile->altModeActive = false;
    profile->mouseResidualX = 0.0f;
    profile->mouseResidualY = 0.0f;
}

void moveProfileResetMouseState(MoveProfile *profile)
{
    int i, j;
    if (!profile) return;
    for (i = 0; i < profile->buttonBindingCount; ++i) {
        for (j = 0; j < profile->buttonBindings[i].actionCount; ++j) {
            profile->buttonBindings[i].actions[j].isDown = false;
            profile->buttonBindings[i].actions[j].lastRepeatTick = 0;
        }
    }
    for (i = 0; i < profile->altButtonBindingCount; ++i) {
        for (j = 0; j < profile->altButtonBindings[i].actionCount; ++j) {
            profile->altButtonBindings[i].actions[j].isDown = false;
            profile->altButtonBindings[i].actions[j].lastRepeatTick = 0;
        }
    }
    for (i = 0; i < SensorAxisInput_Count; ++i) {
        for (j = 0; j < profile->sensorBindings[i].actionCount; ++j) {
            profile->sensorBindings[i].actions[j].isDown = false;
            profile->sensorBindings[i].actions[j].lastRepeatTick = 0;
        }
        for (j = 0; j < profile->altSensorBindings[i].actionCount; ++j) {
            profile->altSensorBindings[i].actions[j].isDown = false;
            profile->altSensorBindings[i].actions[j].lastRepeatTick = 0;
        }
    }
    profile->mouseResidualX = 0.0f;
    profile->mouseResidualY = 0.0f;
}

static void applyButtonBindingArray(ButtonBinding *bindings, int binding_count, unsigned int btns, int ps_trigger, XUSB_REPORT *report, bool gyro_stop_requested[SensorAxisIndex_Count], bool accel_reset_requested[SensorAxisIndex_Count], bool *alt_mode_requested, int repeatMs, int mouseRepeatMs, int wheelRepeatMs, DWORD now)
{
    int i, j, axis;
    for (i = 0; i < binding_count; ++i) {
        ButtonBinding *binding = &bindings[i];
        bool pressed = (binding->input == MoveButtonInput_Trigger) ? (ps_trigger > 0) : ((btns & resolvePsMoveButtonMask(binding->input)) != 0);
        for (j = 0; j < binding->actionCount; ++j) {
            ButtonAction *action = &binding->actions[j];
            switch (action->kind) {
                case ActionKind_StopGyroAll: if (pressed) for (axis = 0; axis < SensorAxisIndex_Count; ++axis) gyro_stop_requested[axis] = true; break;
                case ActionKind_StopGyroX: if (pressed) gyro_stop_requested[SensorAxisIndex_X] = true; break;
                case ActionKind_StopGyroY: if (pressed) gyro_stop_requested[SensorAxisIndex_Y] = true; break;
                case ActionKind_StopGyroZ: if (pressed) gyro_stop_requested[SensorAxisIndex_Z] = true; break;
                case ActionKind_ResetAccelAll: if (pressed) for (axis = 0; axis < SensorAxisIndex_Count; ++axis) accel_reset_requested[axis] = true; break;
                case ActionKind_ResetAccelX: if (pressed) accel_reset_requested[SensorAxisIndex_X] = true; break;
                case ActionKind_ResetAccelY: if (pressed) accel_reset_requested[SensorAxisIndex_Y] = true; break;
                case ActionKind_ResetAccelZ: if (pressed) accel_reset_requested[SensorAxisIndex_Z] = true; break;
                case ActionKind_AltMode: if (pressed && alt_mode_requested) *alt_mode_requested = true; break;
                default: updateDigitalButtonAction(action, pressed, report, ps_trigger, repeatMs, mouseRepeatMs, wheelRepeatMs, now); break;
            }
        }
    }
}

static void captureAccelCenterIfRequested(MoveProfile *profile, const bool accel_reset_requested[SensorAxisIndex_Count], int ax, int ay, int az)
{
    int accel_values[SensorAxisIndex_Count], axis;
    accel_values[0] = ax; accel_values[1] = ay; accel_values[2] = az;
    for (axis = 0; axis < SensorAxisIndex_Count; ++axis) {
        if (accel_reset_requested[axis] && !profile->accelResetLatched[axis]) {
            profile->accelCenter[axis] = accel_values[axis];
            profile->accelCenterCaptured[axis] = true;
            profile->accelResetLatched[axis] = true;
        } else if (!accel_reset_requested[axis]) {
            profile->accelResetLatched[axis] = false;
        }
        if (!profile->accelCenterCaptured[axis]) {
            profile->accelCenter[axis] = accel_values[axis];
            profile->accelCenterCaptured[axis] = true;
        }
    }
}

static int adjustDirectionalSensorValue(int processed_value, SensorAxisInput input)
{
    switch (input) {
        case SensorAxisInput_GyroX_Pos: case SensorAxisInput_GyroY_Pos: case SensorAxisInput_GyroZ_Pos:
        case SensorAxisInput_AccelX_Pos: case SensorAxisInput_AccelY_Pos: case SensorAxisInput_AccelZ_Pos:
            return processed_value > 0 ? processed_value : 0;
        case SensorAxisInput_GyroX_Neg: case SensorAxisInput_GyroY_Neg: case SensorAxisInput_GyroZ_Neg:
        case SensorAxisInput_AccelX_Neg: case SensorAxisInput_AccelY_Neg: case SensorAxisInput_AccelZ_Neg:
            return processed_value < 0 ? -processed_value : 0;
        default:
            return processed_value;
    }
}

static void applySensorMouseDelta(MoveProfile *profile, float delta_x, float delta_y)
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

static void applySensorBindingForProfile(MoveProfile *profile, SensorBinding *binding, int processed_value, const AxisSettings *axis, SensorAxisInput input, bool sensor_stopped, int repeat_ms, int mouseRepeatMs, int wheelRepeatMs, XUSB_REPORT *report, DWORD now_tick)
{
    int j;
    float mouse_dx = 0.0f, mouse_dy = 0.0f;
    int magnitude;
    (void)axis;

    processed_value = adjustDirectionalSensorValue(processed_value, input);
    magnitude = abs(processed_value);
    for (j = 0; j < binding->actionCount; ++j) {
        SensorAction *action = &binding->actions[j];
        bool active = (processed_value != 0);
        switch (action->kind) {
            case ActionKind_MouseMoveX: if (!sensor_stopped && processed_value != 0) mouse_dx += (float)processed_value; break;
            case ActionKind_MouseMoveX_Pos: if (!sensor_stopped && magnitude != 0) mouse_dx += (float)magnitude; break;
            case ActionKind_MouseMoveX_Neg: if (!sensor_stopped && magnitude != 0) mouse_dx -= (float)magnitude; break;
            case ActionKind_MouseMoveY: if (!sensor_stopped && processed_value != 0) mouse_dy += (float)processed_value; break;
            case ActionKind_MouseMoveY_Pos: if (!sensor_stopped && magnitude != 0) mouse_dy += (float)magnitude; break;
            case ActionKind_MouseMoveY_Neg: if (!sensor_stopped && magnitude != 0) mouse_dy -= (float)magnitude; break;
            case ActionKind_XboxAxis: if (!sensor_stopped && processed_value != 0) setXboxAxisValue(report, action->data.xboxAxis, normalizeSensorToThumb(processed_value)); break;
            case ActionKind_XboxAxis_Pos: if (!sensor_stopped && magnitude != 0) setXboxAxisValue(report, action->data.xboxAxis, normalizeSensorToThumb(magnitude)); break;
            case ActionKind_XboxAxis_Neg: if (!sensor_stopped && magnitude != 0) setXboxAxisValue(report, action->data.xboxAxis, normalizeSensorToThumb(-magnitude)); break;
            case ActionKind_XboxTrigger: if (!sensor_stopped && processed_value != 0) setXboxAxisValue(report, action->data.xboxAxis, normalizeSensorToTrigger(processed_value)); break;
            case ActionKind_MouseWheel: if (!sensor_stopped && active && (now_tick - action->lastRepeatTick >= (DWORD)wheelRepeatMs || action->lastRepeatTick == 0)) { sendMouseWheelEvent(processed_value > 0 ? WHEEL_DELTA : -WHEEL_DELTA); action->lastRepeatTick = now_tick; } break;
            case ActionKind_MouseWheel_Pos: if (!sensor_stopped && magnitude != 0) sendMouseWheelEvent(WHEEL_DELTA); break;
            case ActionKind_MouseWheel_Neg: if (!sensor_stopped && magnitude != 0) sendMouseWheelEvent(-WHEEL_DELTA); break;
            case ActionKind_Keyboard:
            case ActionKind_MouseButton:
            case ActionKind_XboxButton:
                applySensorButtonLikeAction(action, active && !sensor_stopped, now_tick, repeat_ms, mouseRepeatMs, wheelRepeatMs, report);
                break;
            default:
                break;
        }
    }
    if ((mouse_dx != 0.0f || mouse_dy != 0.0f) && !sensor_stopped) {
        applySensorMouseDelta(profile, mouse_dx, mouse_dy);
    }
}

void processMoveInputDevice(MoveProfile *profile, PSMove *move, XUSB_REPORT *report, DWORD now)
{
    unsigned int btns;
    int raw_trigger;
    int ps_trigger;
    int gx = 0, gy = 0, gz = 0, ax = 0, ay = 0, az = 0;
    int pgx, pgy, pgz, pax, pay, paz;
    int repeat_ms;
    int mouse_repeat_ms;
    SensorBinding (*active_sensor_bindings)[SensorAxisInput_Count];
    AxisSettings *active_gyro_config;
    AxisSettings *active_accel_config;
    bool gyro_stop_requested[SensorAxisIndex_Count] = { false, false, false };
    bool accel_reset_requested[SensorAxisIndex_Count] = { false, false, false };
    bool alt_mode_requested = false;
    AxisSettings *active_trigger_config;

    if (!profile || !move || !report) return;

    {
        int poll_count = 0;
        while (poll_count < MAX_PSMOVE_POLLS_PER_TICK && psmove_poll(move)) {
            ++poll_count;
        }
    }
    btns = psmove_get_buttons(move);
    raw_trigger = psmove_get_trigger(move);
    psmove_get_gyroscope(move, &gx, &gy, &gz);
    psmove_get_accelerometer(move, &ax, &ay, &az);

    repeat_ms = profile->repeatMs;
    mouse_repeat_ms = profile->mouseRepeatMs;
    ps_trigger = applyTriggerSettings(raw_trigger, &profile->triggerConfig);
    applyButtonBindingArray(
        profile->buttonBindings,
        profile->buttonBindingCount,
        btns,
        ps_trigger,
        report,
        gyro_stop_requested,
        accel_reset_requested,
        &alt_mode_requested,
        profile->repeatMs,
        profile->mouseRepeatMs,
        profile->wheelRepeatMs,
        now
    );
    captureAccelCenterIfRequested(profile, accel_reset_requested, ax, ay, az);

    if (profile->altModeActive != alt_mode_requested) {
        resetMoveSensorBindingArrayState(profile->sensorBindings);
        resetMoveSensorBindingArrayState(profile->altSensorBindings);
        profile->mouseResidualX = 0.0f;
        profile->mouseResidualY = 0.0f;
        profile->altModeActive = alt_mode_requested;
    }
    if (alt_mode_requested) {
        ps_trigger = applyTriggerSettings(raw_trigger, &profile->altTriggerConfig);
        applyButtonBindingArray(
            profile->altButtonBindings,
            profile->altButtonBindingCount,
            btns,
            ps_trigger,
            report,
            gyro_stop_requested,
            accel_reset_requested,
            NULL,
            profile->repeatMs,
            profile->mouseRepeatMs,
            profile->wheelRepeatMs,
            now
        );
    }
    active_sensor_bindings = alt_mode_requested ? &profile->altSensorBindings : &profile->sensorBindings;
    active_trigger_config = alt_mode_requested ? &profile->altTriggerConfig : &profile->triggerConfig;
    ps_trigger = applyTriggerSettings(raw_trigger, active_trigger_config);
    active_gyro_config = alt_mode_requested ? profile->altGyroConfig : profile->gyroConfig;
    active_accel_config = alt_mode_requested ? profile->altAccelConfig : profile->accelConfig;

    pgx = applyAxisDeadzone(gx, &active_gyro_config[SensorAxisIndex_X]);
    pgy = applyAxisDeadzone(gy, &active_gyro_config[SensorAxisIndex_Y]);
    pgz = applyAxisDeadzone(gz, &active_gyro_config[SensorAxisIndex_Z]);
    pax = applyAxisDeadzone(ax - profile->accelCenter[SensorAxisIndex_X], &active_accel_config[SensorAxisIndex_X]);
    pay = applyAxisDeadzone(ay - profile->accelCenter[SensorAxisIndex_Y], &active_accel_config[SensorAxisIndex_Y]);
    paz = applyAxisDeadzone(az - profile->accelCenter[SensorAxisIndex_Z], &active_accel_config[SensorAxisIndex_Z]);

#define AP(inputName,val,cfg,stopped) if ((*active_sensor_bindings)[inputName].actionCount > 0) applySensorBindingForProfile(profile, &(*active_sensor_bindings)[inputName], val, cfg, inputName, stopped, repeat_ms, mouse_repeat_ms, profile->wheelRepeatMs, report, now)
    AP(SensorAxisInput_GyroX, pgx, &active_gyro_config[SensorAxisIndex_X], gyro_stop_requested[SensorAxisIndex_X]);
    AP(SensorAxisInput_GyroX_Pos, pgx, &active_gyro_config[SensorAxisIndex_X], gyro_stop_requested[SensorAxisIndex_X]);
    AP(SensorAxisInput_GyroX_Neg, pgx, &active_gyro_config[SensorAxisIndex_X], gyro_stop_requested[SensorAxisIndex_X]);
    AP(SensorAxisInput_GyroY, pgy, &active_gyro_config[SensorAxisIndex_Y], gyro_stop_requested[SensorAxisIndex_Y]);
    AP(SensorAxisInput_GyroY_Pos, pgy, &active_gyro_config[SensorAxisIndex_Y], gyro_stop_requested[SensorAxisIndex_Y]);
    AP(SensorAxisInput_GyroY_Neg, pgy, &active_gyro_config[SensorAxisIndex_Y], gyro_stop_requested[SensorAxisIndex_Y]);
    AP(SensorAxisInput_GyroZ, pgz, &active_gyro_config[SensorAxisIndex_Z], gyro_stop_requested[SensorAxisIndex_Z]);
    AP(SensorAxisInput_GyroZ_Pos, pgz, &active_gyro_config[SensorAxisIndex_Z], gyro_stop_requested[SensorAxisIndex_Z]);
    AP(SensorAxisInput_GyroZ_Neg, pgz, &active_gyro_config[SensorAxisIndex_Z], gyro_stop_requested[SensorAxisIndex_Z]);
    AP(SensorAxisInput_AccelX, pax, &active_accel_config[SensorAxisIndex_X], false);
    AP(SensorAxisInput_AccelX_Pos, pax, &active_accel_config[SensorAxisIndex_X], false);
    AP(SensorAxisInput_AccelX_Neg, pax, &active_accel_config[SensorAxisIndex_X], false);
    AP(SensorAxisInput_AccelY, pay, &active_accel_config[SensorAxisIndex_Y], false);
    AP(SensorAxisInput_AccelY_Pos, pay, &active_accel_config[SensorAxisIndex_Y], false);
    AP(SensorAxisInput_AccelY_Neg, pay, &active_accel_config[SensorAxisIndex_Y], false);
    AP(SensorAxisInput_AccelZ, paz, &active_accel_config[SensorAxisIndex_Z], false);
    AP(SensorAxisInput_AccelZ_Pos, paz, &active_accel_config[SensorAxisIndex_Z], false);
    AP(SensorAxisInput_AccelZ_Neg, paz, &active_accel_config[SensorAxisIndex_Z], false);
#undef AP
}
