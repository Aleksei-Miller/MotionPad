#include "device_manager.h"
#include "rumble_manager.h"
#include "logger.h"
#include "psnavigator.h"
#include "move_input.h"

static PSNavigator *deviceConnectNavigatorByPathWithResult(const char *device_path, PSNavResult *out_result)
{
    PSNavigator *nav;

    if (out_result) {
        *out_result = PSNAV_RESULT_ERROR;
    }

    if (!device_path || !device_path[0]) {
        if (out_result) {
            *out_result = PSNAV_RESULT_INVALID_ARGUMENT;
        }
        return NULL;
    }

    nav = psnavigatorConnectByPath(device_path);
    if (!nav) {
        if (out_result) {
            *out_result = PSNAV_RESULT_OPEN_FAILED;
        }
        return NULL;
    }

    if (out_result) {
        *out_result = PSNAV_RESULT_OK;
    }

    return nav;
}

static bool isNavigatorPathAssigned(const AppContext *app, const char *device_path)
{
    int navigator_index;

    if (!app || !device_path || !device_path[0]) {
        return false;
    }

    for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
        if (app->navigators[navigator_index] &&
            strcmp(app->navigator_paths[navigator_index], device_path) == 0) {
            return true;
        }
    }

    return false;
}

static bool isMoveSourceIdAssigned(const AppContext *app, int source_id)
{
    int move_index;
    if (!app) return false;
    for (move_index = 0; move_index < MOVE_COUNT; ++move_index) {
        if (app->move_connected[move_index] && app->move_source_id[move_index] == source_id) return true;
    }
    return false;
}

static bool isMoveSerialAssigned(const AppContext *app, const char *serial)
{
    int move_index;

    if (!app || !serial || !serial[0]) return false;

    for (move_index = 0; move_index < MOVE_COUNT; ++move_index) {
        if (app->move_connected[move_index] &&
            strcmp(app->move_serials[move_index], serial) == 0) {
            return true;
        }
    }

    return false;
}

static void deviceHandleNavigatorDisconnect(AppContext *app, int navigator_index)
{
    char device_path[NAVIGATOR_PATH_LENGTH];

    if (!app || navigator_index < 0 || navigator_index >= NAVIGATOR_COUNT || !app->navigators[navigator_index]) {
        return;
    }

    strncpy(device_path, app->navigator_paths[navigator_index], sizeof(device_path) - 1);
    device_path[sizeof(device_path) - 1] = '\0';

    psnavigatorDisconnect(app->navigators[navigator_index]);
    app->navigators[navigator_index] = NULL;
    app->navigator_paths[navigator_index][0] = '\0';
    navigatorProfileResetRuntimeState(&app->navigator_profiles[navigator_index]);
    logWrite("navigator", "Navigator%d disconnected: %s", navigator_index + 1, device_path[0] ? device_path : "<unknown>");
}

void deviceSetMoveDisconnected(AppContext *app, int move_index)
{
    bool was_connected;

    if (!app || move_index < 0 || move_index >= MOVE_COUNT) return;

    was_connected = app->move_connected[move_index] || app->moves[move_index] != NULL;

    if (app->moves[move_index]) {
        rumbleSetMove(app->moves[move_index], 0);
        psmove_disconnect(app->moves[move_index]);
        app->moves[move_index] = NULL;
    }

    app->move_connected[move_index] = false;
    app->move_source_id[move_index] = -1;
    app->move_serials[move_index][0] = '\0';
    moveProfileResetRuntimeState(&app->move_profiles[move_index]);
    moveProfileResetMouseState(&app->move_profiles[move_index]);

    if (was_connected) {
        logWrite("psmove", "Move%d disconnected", move_index + 1);
    }
}

void deviceRemoveDisconnectedMovesBestEffort(AppContext *app)
{
    int count;
    int connected_slots = 0;
    int move_index;

    if (!app) return;

    count = psmove_count_connected();

    for (move_index = 0; move_index < MOVE_COUNT; ++move_index) {
        if (app->move_connected[move_index]) ++connected_slots;
    }

    if (count < connected_slots) {
        for (move_index = MOVE_COUNT - 1; move_index >= 0; --move_index) {
            if (app->move_connected[move_index]) {
                deviceSetMoveDisconnected(app, move_index);
                ++count;
                if (count >= connected_slots) break;
            }
        }
    }
}

void deviceConnectAvailableMoves(AppContext *app)
{
    int count;
    int index;

    if (!app) return;

    count = psmove_count_connected();
    if (count <= 0) return;

    for (index = 0; index < count; ++index) {
        PSMove *move = NULL;
        char *serial = NULL;
        int target_slot = -1;

        if (app->move_connected[0] && app->move_connected[1]) return;
        if (isMoveSourceIdAssigned(app, index)) continue;

        if (!app->move_connected[0]) target_slot = 0;
        else if (!app->move_connected[1]) target_slot = 1;

        if (target_slot < 0) break;

        move = psmove_connect_by_id(index);
        if (!move) continue;

        serial = psmove_get_serial(move);
        if (isMoveSerialAssigned(app, serial)) {
            if (serial) psmove_free_mem(serial);
            psmove_disconnect(move);
            continue;
        }

        app->moves[target_slot] = move;
        app->move_connected[target_slot] = true;
        app->move_source_id[target_slot] = index;
        if (serial && serial[0]) {
            strncpy(app->move_serials[target_slot], serial, MOVE_SERIAL_LENGTH - 1);
            app->move_serials[target_slot][MOVE_SERIAL_LENGTH - 1] = '\0';
        } else {
            app->move_serials[target_slot][0] = '\0';
        }

        if (serial) psmove_free_mem(serial);

        moveProfileResetMouseState(&app->move_profiles[target_slot]);

        logWrite(
            "psmove",
            "Move%d connected (id=%d, serial=%s)",
            target_slot + 1,
            index,
            app->move_serials[target_slot][0] ? app->move_serials[target_slot] : "<unknown>"
        );
    }
}

void deviceConnectNavigatorsIfNeeded(AppContext *app)
{
    char device_path[NAVIGATOR_PATH_LENGTH];
    DWORD now;
    int device_count;
    int device_id;
    int navigator_index;

    if (!app) {
        return;
    }

    for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
        if (app->navigators[navigator_index] && !psnavigatorIsConnected(app->navigators[navigator_index])) {
            deviceHandleNavigatorDisconnect(app, navigator_index);
        }
    }

    now = GetTickCount();

    if (app->navigator_reconnect_tick != 0 &&
        (DWORD)(now - app->navigator_reconnect_tick) < 500) {
        return;
    }

    app->navigator_reconnect_tick = now;

    device_count = psnavigatorGetDeviceCount();
    if (device_count <= 0) {
        return;
    }

    for (device_id = 0; device_id < device_count; ++device_id) {
        PSNavResult path_result;
        PSNavResult connect_result;
        int target_slot = -1;

        path_result = psnavigatorGetDevicePathById(device_id, device_path, (int)sizeof(device_path));
        if (path_result != PSNAV_RESULT_OK) {
            logWrite(
                "navigator",
                "Navigator enumeration failed for id=%d: %d (%s): %s",
                device_id,
                (int)path_result,
                psnavigatorResultToString(path_result),
                psnavigatorGetLastError(NULL)
            );
            continue;
        }

        if (isNavigatorPathAssigned(app, device_path)) {
            continue;
        }

        for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
            if (!app->navigators[navigator_index]) {
                target_slot = navigator_index;
                break;
            }
        }

        if (target_slot < 0) {
            return;
        }

        app->navigators[target_slot] = deviceConnectNavigatorByPathWithResult(device_path, &connect_result);
        if (app->navigators[target_slot]) {
            navigatorProfileResetRuntimeState(&app->navigator_profiles[target_slot]);
            strncpy(app->navigator_paths[target_slot], device_path, sizeof(app->navigator_paths[target_slot]) - 1);
            app->navigator_paths[target_slot][sizeof(app->navigator_paths[target_slot]) - 1] = '\0';
            logWrite(
                "navigator",
                "Navigator%d connected (id=%d): %s",
                target_slot + 1,
                device_id,
                app->navigator_paths[target_slot]
            );
        } else {
            logWrite(
                "navigator",
                "Navigator connect failed for id=%d: %d (%s): %s",
                device_id,
                (int)connect_result,
                psnavigatorResultToString(connect_result),
                psnavigatorGetLastError(NULL)
            );
        }
    }
}

bool deviceHasAnyInputConnected(const AppContext *app)
{
    int navigator_index;
    if (!app) return false;
    if (app->move_connected[0] || app->move_connected[1]) return true;
    for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
        if (app->navigators[navigator_index]) return true;
    }
    return false;
}
