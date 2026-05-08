#ifndef RUMBLE_MANAGER_H
#define RUMBLE_MANAGER_H

#include "app_context.h"

void rumbleSetMove(PSMove *move, UCHAR rumble);
UCHAR rumbleAdaptMotor(const AppContext *app, UCHAR motor_value, const RumbleMotorConfig *cfg);
UCHAR rumbleCombineValues(UCHAR large_rumble, UCHAR small_rumble);
VOID CALLBACK rumbleHandleXboxCallback(PVIGEM_CLIENT client, PVIGEM_TARGET target, UCHAR large_motor, UCHAR small_motor, UCHAR led_number, LPVOID user_data);

#endif
