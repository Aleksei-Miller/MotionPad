#ifndef PROFILE_WATCHER_H
#define PROFILE_WATCHER_H

#include "app_context.h"

bool profileWatcherStart(AppContext *app);
void profileWatcherStop(AppContext *app);
bool settingsWatcherStart(AppContext *app);
void settingsWatcherStop(AppContext *app);

#endif
