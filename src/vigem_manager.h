#ifndef VIGEM_MANAGER_H
#define VIGEM_MANAGER_H

#include "app_context.h"

bool vigemEnsureClient(AppContext *app);
bool vigemEnsurePad(AppContext *app);
bool vigemSubmitReport(AppContext *app, const XUSB_REPORT *report);
void vigemDestroyPad(AppContext *app);
void vigemShutdown(AppContext *app);

#endif
