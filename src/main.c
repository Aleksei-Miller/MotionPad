#include "app_context.h"
#include "profile.h"
#include "device_manager.h"
#include "tray_ui.h"
#include "vigem_manager.h"
#include "logger.h"
#include "profile_watcher.h"
#include "psnavigator.h"
#include "telemetry.h"

#define TRAY_UPDATE_INTERVAL_MS 1000

static bool fileExistsW(const wchar_t *path)
{
    DWORD attributes;

    if (!path || path[0] == L'\0') {
        return false;
    }

    attributes = GetFileAttributesW(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static void fallbackToDefaultProfileIfMissing(AppContext *app)
{
    wchar_t default_path[MAX_PATH];

    if (!app || fileExistsW(app->config_path)) {
        return;
    }

    profileBuildPath(default_path, MAX_PATH, L"profiles\\default.ini");
    if (_wcsicmp(app->config_path, default_path) == 0) {
        logWriteW("config", L"active profile is missing and default profile is already selected: %ls", app->config_path);
        return;
    }

    logWriteW("config", L"active profile was deleted, switching to default: %ls", default_path);
    profileWatcherStop(app);
    wcsncpy(app->config_path, default_path, MAX_PATH - 1);
    app->config_path[MAX_PATH - 1] = L'\0';
    profileSaveSettings(app);
    if (!profileWatcherStart(app)) {
        logWriteW("config", L"failed to start watcher for fallback profile: %ls", app->config_path);
    }
}

static void updateServiceDevices(AppContext *app, DWORD *last_service_tick)
{
    if ((DWORD)(GetTickCount() - *last_service_tick) >= SERVICE_POLL_MS) {
        deviceRemoveDisconnectedMovesBestEffort(app);
        deviceConnectAvailableMoves(app);
        deviceConnectNavigatorsIfNeeded(app);
        *last_service_tick = GetTickCount();
    }
}

static void updateTrayTooltipIfNeeded(AppContext *app, DWORD *last_tray_tick)
{
    if (GetTickCount() - *last_tray_tick >= TRAY_UPDATE_INTERVAL_MS) {
        trayUpdateTooltip(app);
        *last_tray_tick = GetTickCount();
    }
}

static void maybeSendTelemetry(AppContext *app)
{
    if (!profileIsTelemetryEnabled(app) || !telemetryIsConnected()) {
        return;
    }

    TelemetryData data = {0};
    data.tick = GetTickCount();

    if (app->moves[0]) {
        while (psmove_poll(app->moves[0])) { }
        data.m1.con = psmove_connection_type(app->moves[0]);
        data.m1.bat = psmove_get_battery(app->moves[0]);
        data.m1.btns = psmove_get_buttons(app->moves[0]);
        data.m1.trig = psmove_get_trigger(app->moves[0]);
        psmove_get_gyroscope(app->moves[0], &data.m1.gyro[0], &data.m1.gyro[1], &data.m1.gyro[2]);
        psmove_get_accelerometer(app->moves[0], &data.m1.accel[0], &data.m1.accel[1], &data.m1.accel[2]);
    }

    if (app->moves[1]) {
        while (psmove_poll(app->moves[1])) { }
        data.m2.con = psmove_connection_type(app->moves[1]);
        data.m2.bat = psmove_get_battery(app->moves[1]);
        data.m2.btns = psmove_get_buttons(app->moves[1]);
        data.m2.trig = psmove_get_trigger(app->moves[1]);
        psmove_get_gyroscope(app->moves[1], &data.m2.gyro[0], &data.m2.gyro[1], &data.m2.gyro[2]);
        psmove_get_accelerometer(app->moves[1], &data.m2.accel[0], &data.m2.accel[1], &data.m2.accel[2]);
    }

    if (app->navigators[0]) {
        psnavigatorPoll(app->navigators[0]);
        data.n1.con = psnavigatorGetConnectionType(app->navigators[0]);
        data.n1.bat = psnavigatorGetBattery(app->navigators[0]);
        data.n1.btns = psnavigatorGetButtons(app->navigators[0]);
        data.n1.trig = psnavigatorGetAxis(app->navigators[0], PSNAV_AXIS_TRIGGER);
        data.n1.x = psnavigatorGetAxis(app->navigators[0], PSNAV_AXIS_STICK_X);
        data.n1.y = psnavigatorGetAxis(app->navigators[0], PSNAV_AXIS_STICK_Y);
    }

    if (app->navigators[1]) {
        psnavigatorPoll(app->navigators[1]);
        data.n2.con = psnavigatorGetConnectionType(app->navigators[1]);
        data.n2.bat = psnavigatorGetBattery(app->navigators[1]);
        data.n2.btns = psnavigatorGetButtons(app->navigators[1]);
        data.n2.trig = psnavigatorGetAxis(app->navigators[1], PSNAV_AXIS_TRIGGER);
        data.n2.x = psnavigatorGetAxis(app->navigators[1], PSNAV_AXIS_STICK_X);
        data.n2.y = psnavigatorGetAxis(app->navigators[1], PSNAV_AXIS_STICK_Y);
    }

    telemetrySend(&data);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show)
{
    HANDLE single_instance_mutex = NULL;
    AppContext app;
    int navigator_index;

    DWORD last_tray_update_tick = 0;
    DWORD last_service_poll_tick = 0;
    (void)instance;
    (void)prev_instance;
    (void)cmd_line;
    (void)cmd_show;

    single_instance_mutex = CreateMutexW(NULL, TRUE, L"Global\\PSMove4PC_SingleInstance");
    if (!single_instance_mutex) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(single_instance_mutex);
        return 0;
    }

    loggerInit();
    logWrite("main", "program started");
    telemetryInit();

    appContextInit(&app);
    profileLoadSettings(&app);
    profileLoadAll(&app);

    if (!trayInitWindow(&app)) goto cleanup;
    if (!trayInitIcon(&app)) goto cleanup;
    if (!profileWatcherStart(&app)) {
        logWriteW("config", L"failed to start watcher for: %ls", app.config_path);
        goto cleanup;
    }

    trayUpdateTooltip(&app);
    last_tray_update_tick = GetTickCount();
    last_service_poll_tick = last_tray_update_tick;

    psmove_init(PSMOVE_CURRENT_VERSION);
    psnavigatorInit(PSNAVIGATOR_API_VERSION);
    logWrite("main", "started. waiting for PS Move and PS Navigator controllers...");

    while (InterlockedCompareExchange(&app.running, 1, 1)) {
        trayHandleMessageLoop(&app);
        if (!InterlockedCompareExchange(&app.running, 1, 1)) break;
        trayApplyPendingProfileSelection(&app);

        if (InterlockedExchange(&app.config_dirty, 0)) {
            fallbackToDefaultProfileIfMissing(&app);
            profileLoadAll(&app);
        }

        updateServiceDevices(&app, &last_service_poll_tick);
        updateTrayTooltipIfNeeded(&app, &last_tray_update_tick);

        if (!app.emulation_enabled) {
            appContextReleaseSyntheticInputs(&app);
            vigemDestroyPad(&app);
            Sleep(WAIT_SLEEP_MS);
            continue;
        }
        if (!deviceHasAnyInputConnected(&app)) {
            vigemDestroyPad(&app);
            Sleep(WAIT_SLEEP_MS);
            continue;
        }
        if (!vigemEnsureClient(&app)) {
            Sleep(WAIT_SLEEP_MS);
            continue;
        }
        if (!vigemEnsurePad(&app)) {
            Sleep(WAIT_SLEEP_MS);
            continue;
        }

        appContextZeroReport(&app);
        if (app.move_connected[0] && app.moves[0]) {
            processMoveInputDevice(&app.move_profiles[0], app.moves[0], &app.report);
        }
        if (app.move_connected[1] && app.moves[1]) {
            processMoveInputDevice(&app.move_profiles[1], app.moves[1], &app.report);
        }
        for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
            if (app.navigators[navigator_index]) {
                processNavigatorInputDevice(
                    app.navigators[navigator_index],
                    &app.navigator_profiles[navigator_index],
                    &app.report
                );
            }
        }

        maybeSendTelemetry(&app);

        if (app.vigem_client && app.vigem_pad) {
            if (!vigemSubmitReport(&app, &app.report)) {
                Sleep(100);
                continue;
            }
        }
        Sleep(LOOP_SLEEP_MS);
    }

cleanup:
    logWrite("main", "shutting down...");
    telemetryShutdown();
    profileWatcherStop(&app);
    appContextStopEmulation(&app);
    deviceSetMoveDisconnected(&app, 0);
    deviceSetMoveDisconnected(&app, 1);
    for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
        if (app.navigators[navigator_index]) {
            psnavigatorDisconnect(app.navigators[navigator_index]);
            app.navigators[navigator_index] = NULL;
            app.navigator_paths[navigator_index][0] = '\0';
        }
    }
    psnavigatorShutdown();
    if (app.tray_added) trayRemoveIcon(&app);
    vigemShutdown(&app);
    trayShutdownWindow(&app);
    logWrite("main", "program stopped");
    loggerClose();

    if (single_instance_mutex) {
        ReleaseMutex(single_instance_mutex);
        CloseHandle(single_instance_mutex);
        single_instance_mutex = NULL;
    }

    return 0;
}

