#include "app_context.h"
#include "rumble_manager.h"
#include "vigem_manager.h"

void appContextInit(AppContext *app)
{
    int move_index;
    int navigator_index;
    if (!app) return;
    ZeroMemory(app, sizeof(*app));
    app->output_enabled = true;
    app->running = 1;
    app->config_dirty = 0;
    app->profile_switch_pending = 0;
    app->config_watch_thread = NULL;
    app->config_watch_stop_event = NULL;
    app->vigem_notification_registered = false;
    for (move_index = 0; move_index < MOVE_COUNT; ++move_index) {
        app->move_source_id[move_index] = -1;
        app->move_serials[move_index][0] = '\0';
        app->move_battery_raw[move_index] = -1;
    }
    app->navigator_reconnect_tick = 0;
    for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
        app->navigators[navigator_index] = NULL;
        app->navigator_paths[navigator_index][0] = '\0';
    }
}

void appContextReleaseSyntheticInputs(AppContext *app)
{
    int move_index;
    int navigator_index;
    if (!app) return;
    for (move_index = 0; move_index < MOVE_COUNT; ++move_index) {
        moveProfileResetRuntimeState(&app->move_profiles[move_index]);
    }
    for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
        navigatorProfileResetRuntimeState(&app->navigator_profiles[navigator_index]);
    }
}

void appContextZeroReport(AppContext *app)
{
    if (!app) return;
    ZeroMemory(&app->report, sizeof(app->report));
}

void appContextResetInputsKeepPad(AppContext *app)
{
    int move_index;
    if (!app) return;
    appContextReleaseSyntheticInputs(app);
    for (move_index = 0; move_index < MOVE_COUNT; ++move_index) {
        if (app->moves[move_index]) {
            rumbleSetMove(app->moves[move_index], 0);
        }
    }
    appContextZeroReport(app);
    vigemSubmitReport(app, &app->report);
}

void appContextStopEmulation(AppContext *app)
{
    if (!app) return;
    appContextResetInputsKeepPad(app);
    vigemDestroyPad(app);
}
