#include "app_context.h"
#include "profile.h"
#include "device_manager.h"
#include "tray_ui.h"
#include "vigem_manager.h"
#include "logger.h"
#include "profile_watcher.h"
#include "nav_device.h"
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

static void updateServiceDevices(AppContext *app, DWORD *last_service_tick, DWORD now)
{
    if ((DWORD)(now - *last_service_tick) >= SERVICE_POLL_MS) {
        deviceRemoveDisconnectedMovesBestEffort(app);
        deviceConnectAvailableMoves(app);
        deviceConnectNavigatorsIfNeeded(app, now);
        app->move_count_cache = psmove_count_connected();
        *last_service_tick = now;
    }
}

static void updateTrayTooltipIfNeeded(AppContext *app, DWORD *last_tray_tick, DWORD now)
{
    if (now - *last_tray_tick >= TRAY_UPDATE_INTERVAL_MS) {
        trayUpdateTooltip(app);
        *last_tray_tick = now;
    }
}

static void maybeSendTelemetry(AppContext *app, DWORD now)
{
    if (!profileIsTelemetryEnabled(app) || !telemetryIsConnected()) {
        return;
    }

    TelemetryData data = {0};
    data.tick = now;

    if (app->moves[0]) {
        data.m1.con = psmove_connection_type(app->moves[0]);
        data.m1.bat = psmove_get_battery(app->moves[0]);
        data.m1.btns = psmove_get_buttons(app->moves[0]);
        data.m1.trig = psmove_get_trigger(app->moves[0]);
        psmove_get_gyroscope(app->moves[0], &data.m1.gyro[0], &data.m1.gyro[1], &data.m1.gyro[2]);
        psmove_get_accelerometer(app->moves[0], &data.m1.accel[0], &data.m1.accel[1], &data.m1.accel[2]);
    }

    if (app->moves[1]) {
        data.m2.con = psmove_connection_type(app->moves[1]);
        data.m2.bat = psmove_get_battery(app->moves[1]);
        data.m2.btns = psmove_get_buttons(app->moves[1]);
        data.m2.trig = psmove_get_trigger(app->moves[1]);
        psmove_get_gyroscope(app->moves[1], &data.m2.gyro[0], &data.m2.gyro[1], &data.m2.gyro[2]);
        psmove_get_accelerometer(app->moves[1], &data.m2.accel[0], &data.m2.accel[1], &data.m2.accel[2]);
    }

    if (app->navigators[0]) {
        data.n1.con = navDeviceGetConnectionType(app->navigators[0]);
        data.n1.bat = navDeviceGetBattery(app->navigators[0]);
        data.n1.btns = navDeviceGetButtons(app->navigators[0]);
        data.n1.trig = navDeviceGetAxis(app->navigators[0], NavDevAxis_TRIGGER);
        data.n1.x = navDeviceGetAxis(app->navigators[0], NavDevAxis_STICK_X);
        data.n1.y = navDeviceGetAxis(app->navigators[0], NavDevAxis_STICK_Y);
    }

    if (app->navigators[1]) {
        data.n2.con = navDeviceGetConnectionType(app->navigators[1]);
        data.n2.bat = navDeviceGetBattery(app->navigators[1]);
        data.n2.btns = navDeviceGetButtons(app->navigators[1]);
        data.n2.trig = navDeviceGetAxis(app->navigators[1], NavDevAxis_TRIGGER);
        data.n2.x = navDeviceGetAxis(app->navigators[1], NavDevAxis_STICK_X);
        data.n2.y = navDeviceGetAxis(app->navigators[1], NavDevAxis_STICK_Y);
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
    DWORD last_liveness_tick = 0;
    (void)instance;
    (void)prev_instance;
    (void)cmd_line;
    (void)cmd_show;

    single_instance_mutex = CreateMutexW(NULL, TRUE, L"Global\\MotionPad_SingleInstance");
    if (!single_instance_mutex) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(single_instance_mutex);
        return 0;
    }

    loggerInit();
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
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
    if (!settingsWatcherStart(&app)) {
        logWriteW("settings", L"failed to start watcher for: %ls", app.settings_path);
        goto cleanup;
    }

    trayUpdateTooltip(&app);
    last_tray_update_tick = GetTickCount();
    last_service_poll_tick = last_tray_update_tick;

    psmove_init(PSMOVE_CURRENT_VERSION);
    navDeviceGlobalInit(app.use_bthps3 ? NavBackend_SDL3 : NavBackend_LibNav);
    logWrite("main", "started. waiting for PS Move and PS Navigator controllers...");

    while (InterlockedCompareExchange(&app.running, 1, 1)) {
        DWORD now = GetTickCount();

        trayHandleMessageLoop(&app);
        if (!InterlockedCompareExchange(&app.running, 1, 1)) break;
        trayApplyPendingProfileSelection(&app);
        trayCheckAutoProfile(&app, now);

        if (InterlockedExchange(&app.config_dirty, 0)) {
            fallbackToDefaultProfileIfMissing(&app);
            profileLoadAll(&app);
        }

        if (InterlockedExchange(&app.settings_dirty, 0)) {
            wchar_t old_path[MAX_PATH];
            wcsncpy(old_path, app.config_path, MAX_PATH - 1);
            old_path[MAX_PATH - 1] = L'\0';
            profileReloadSettings(&app);
            if (_wcsicmp(old_path, app.config_path) != 0) {
                profileWatcherStop(&app);
                fallbackToDefaultProfileIfMissing(&app);
                profileLoadAll(&app);
                if (!profileWatcherStart(&app)) {
                    logWriteW("config", L"failed to restart watcher for: %ls", app.config_path);
                }
            }
        }

        updateServiceDevices(&app, &last_service_poll_tick, now);
        updateTrayTooltipIfNeeded(&app, &last_tray_update_tick, now);

        if ((DWORD)(now - last_liveness_tick) >= DEVICE_LIVENESS_POLL_MS) {
            deviceCheckAlive(&app, now);
            last_liveness_tick = now;
        }

        if (!app.output_enabled) {
            appContextReleaseSyntheticInputs(&app);
            vigemDestroyPad(&app);
            Sleep(WAIT_SLEEP_MS);
            continue;
        }
        if (!deviceHasAnyInputConnected(&app)) {
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
        navDeviceFrameStart();
        for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
            if (app.navigators[navigator_index]) {
                processNavigatorInputDevice(
                    app.navigators[navigator_index],
                    &app.navigator_profiles[navigator_index],
                    &app.report,
                    now
                );
            }
        }
        if (app.move_connected[0] && app.moves[0]) {
            processMoveInputDevice(&app.move_profiles[0], app.moves[0], &app.report, now);
            app.move_battery_raw[0] = psmove_get_battery(app.moves[0]);
        }
        if (app.move_connected[1] && app.moves[1]) {
            processMoveInputDevice(&app.move_profiles[1], app.moves[1], &app.report, now);
            app.move_battery_raw[1] = psmove_get_battery(app.moves[1]);
        }
        maybeSendTelemetry(&app, now);
        if (app.vigem_client && app.vigem_pad) {
            if (!vigemSubmitReport(&app, &app.report)) {
                Sleep(WAIT_SLEEP_MS);
                continue;
            }
        }
        Sleep(app.poll_rate_ms);
    }

cleanup:
    logWrite("main", "shutting down...");
    telemetryShutdown();
    settingsWatcherStop(&app);
    profileWatcherStop(&app);
    appContextStopEmulation(&app);
    deviceSetMoveDisconnected(&app, 0);
    deviceSetMoveDisconnected(&app, 1);
    for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
        if (app.navigators[navigator_index]) {
            navDeviceDestroy(app.navigators[navigator_index]);
            app.navigators[navigator_index] = NULL;
            app.navigator_paths[navigator_index][0] = '\0';
        }
    }
    navDeviceGlobalShutdown();
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

