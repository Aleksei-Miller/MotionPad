#ifndef TRAY_UI_H
#define TRAY_UI_H

#include "app_context.h"

bool trayInitWindow(AppContext *app);
bool trayInitIcon(AppContext *app);
void trayRemoveIcon(AppContext *app);
void trayShutdownWindow(AppContext *app);
void trayToggleEmulation(AppContext *app);
void trayHandleMessageLoop(AppContext *app);
void trayApplyPendingProfileSelection(AppContext *app);
void trayCheckAutoProfile(AppContext *app, DWORD now);
void trayUpdateTooltip(AppContext *app);

#endif
