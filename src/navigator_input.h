#ifndef NAVIGATOR_INPUT_H
#define NAVIGATOR_INPUT_H

#include "nav_device.h"
#include "input_actions.h"

typedef enum NavigatorButtonInput {
    NavigatorButtonInput_Cross = 0,
    NavigatorButtonInput_Circle,
    NavigatorButtonInput_L1,
    NavigatorButtonInput_L2,
    NavigatorButtonInput_L3,
    NavigatorButtonInput_PS,
    NavigatorButtonInput_Up,
    NavigatorButtonInput_Right,
    NavigatorButtonInput_Down,
    NavigatorButtonInput_Left,
    NavigatorButtonInput_Count
} NavigatorButtonInput;

typedef enum NavigatorAxisInput {
    NavigatorAxisInput_X = 0,
    NavigatorAxisInput_X_Pos,
    NavigatorAxisInput_X_Neg,
    NavigatorAxisInput_Y,
    NavigatorAxisInput_Y_Pos,
    NavigatorAxisInput_Y_Neg,
    NavigatorAxisInput_Count
} NavigatorAxisInput;

typedef struct NavigatorButtonBinding {
    NavigatorButtonInput input;
    ButtonAction actions[MAX_ACTIONS_PER_INPUT];
    int actionCount;
} NavigatorButtonBinding;

typedef struct NavigatorSensorBinding {
    NavigatorAxisInput input;
    SensorAction actions[MAX_SENSOR_ACTIONS];
    int actionCount;
} NavigatorSensorBinding;

typedef struct NavigatorProfile {
    NavigatorButtonBinding buttonBindings[MAX_BOUND_INPUTS];
    int buttonBindingCount;
    NavigatorButtonBinding altButtonBindings[MAX_BOUND_INPUTS];
    int altButtonBindingCount;
    NavigatorSensorBinding sensorBindings[NavigatorAxisInput_Count];
    NavigatorSensorBinding altSensorBindings[NavigatorAxisInput_Count];
    AxisSettings triggerConfig;
    AxisSettings altTriggerConfig;
    AxisSettings stickConfig[2];
    AxisSettings altStickConfig[2];
    int repeatMs;
    int mouseRepeatMs;
    int wheelRepeatMs;
    bool altModeActive;
    float mouseResidualX;
    float mouseResidualY;
} NavigatorProfile;

void navigatorProfileResetRuntimeState(NavigatorProfile *profile);
void processNavigatorInputDevice(NavDevice *nav, NavigatorProfile *profile, XUSB_REPORT *report, DWORD now);

#endif
