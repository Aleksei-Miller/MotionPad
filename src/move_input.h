#ifndef MOVE_INPUT_H
#define MOVE_INPUT_H

#include "input_actions.h"

typedef enum MoveButtonInput {
    MoveButtonInput_Cross = 0,
    MoveButtonInput_Circle,
    MoveButtonInput_Square,
    MoveButtonInput_Triangle,
    MoveButtonInput_Select,
    MoveButtonInput_Start,
    MoveButtonInput_PS,
    MoveButtonInput_Move,
    MoveButtonInput_Trigger,
    MoveButtonInput_Count
} MoveButtonInput;

typedef enum SensorAxisInput {
    SensorAxisInput_GyroX = 0,
    SensorAxisInput_GyroX_Pos,
    SensorAxisInput_GyroX_Neg,
    SensorAxisInput_GyroY,
    SensorAxisInput_GyroY_Pos,
    SensorAxisInput_GyroY_Neg,
    SensorAxisInput_GyroZ,
    SensorAxisInput_GyroZ_Pos,
    SensorAxisInput_GyroZ_Neg,
    SensorAxisInput_AccelX,
    SensorAxisInput_AccelX_Pos,
    SensorAxisInput_AccelX_Neg,
    SensorAxisInput_AccelY,
    SensorAxisInput_AccelY_Pos,
    SensorAxisInput_AccelY_Neg,
    SensorAxisInput_AccelZ,
    SensorAxisInput_AccelZ_Pos,
    SensorAxisInput_AccelZ_Neg,
    SensorAxisInput_Count
} SensorAxisInput;

typedef enum SensorAxisIndex {
    SensorAxisIndex_X = 0,
    SensorAxisIndex_Y,
    SensorAxisIndex_Z,
    SensorAxisIndex_Count
} SensorAxisIndex;

typedef struct ButtonBinding {
    MoveButtonInput input;
    ButtonAction actions[MAX_ACTIONS_PER_INPUT];
    int actionCount;
} ButtonBinding;

typedef struct SensorBinding {
    SensorAxisInput input;
    SensorAction actions[MAX_SENSOR_ACTIONS];
    int actionCount;
} SensorBinding;

typedef struct MoveProfile {
    ButtonBinding buttonBindings[MAX_BOUND_INPUTS];
    int buttonBindingCount;
    ButtonBinding altButtonBindings[MAX_BOUND_INPUTS];
    int altButtonBindingCount;
    SensorBinding sensorBindings[SensorAxisInput_Count];
    SensorBinding altSensorBindings[SensorAxisInput_Count];
    AxisSettings triggerConfig;
    AxisSettings altTriggerConfig;
    AxisSettings gyroConfig[SensorAxisIndex_Count];
    AxisSettings accelConfig[SensorAxisIndex_Count];
    AxisSettings altGyroConfig[SensorAxisIndex_Count];
    AxisSettings altAccelConfig[SensorAxisIndex_Count];
    int repeatMs;
    int wheelRepeatMs;
    int accelCenter[SensorAxisIndex_Count];
    bool accelCenterCaptured[SensorAxisIndex_Count];
    bool accelResetLatched[SensorAxisIndex_Count];
    bool altModeActive;
    float mouseResidualX;
    float mouseResidualY;
} MoveProfile;

void moveProfileResetRuntimeState(MoveProfile *profile);
void moveProfileResetMouseState(MoveProfile *profile);
void processMoveInputDevice(MoveProfile *profile, PSMove *move, XUSB_REPORT *report);

#endif
