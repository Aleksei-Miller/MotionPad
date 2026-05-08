#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "app_context.h"

void deviceSetMoveDisconnected(AppContext *app, int move_index);
void deviceRemoveDisconnectedMovesBestEffort(AppContext *app);
void deviceConnectAvailableMoves(AppContext *app);
void deviceConnectNavigatorsIfNeeded(AppContext *app);
bool deviceHasAnyInputConnected(const AppContext *app);

#endif
