#include "device_manager.h"
#include "rumble_manager.h"
#include "vigem_manager.h"
#include "logger.h"
#include "nav_device.h"
#include "move_input.h"

static NavDevice *deviceConnectNavigatorByIdentifier(const char *identifier)
{
    if (!identifier || !identifier[0]) return NULL;

    return navDeviceCreate(identifier);
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

static bool isSerialEmptyOrPlaceholder(const char *s)
{
    return !s || !s[0] || strcmp(s, "00:00:00:00:00:00") == 0;
}

static bool isMoveSerialAssigned(const AppContext *app, const char *serial)
{
    int move_index;

    if (!app || isSerialEmptyOrPlaceholder(serial)) return false;

    for (move_index = 0; move_index < MOVE_COUNT; ++move_index) {
        if (app->move_connected[move_index] &&
            strcmp(app->move_serials[move_index], serial) == 0) {
            return true;
        }
    }

    return false;
}

static void deviceHandleNavigatorDisconnect(AppContext *app, int navigator_index, DWORD now)
{
    char device_path[NAVIGATOR_PATH_LENGTH];

    if (!app || navigator_index < 0 || navigator_index >= NAVIGATOR_COUNT || !app->navigators[navigator_index]) {
        return;
    }

    strncpy(device_path, app->navigator_paths[navigator_index], sizeof(device_path) - 1);
    device_path[sizeof(device_path) - 1] = '\0';

    navDeviceDestroy(app->navigators[navigator_index]);
    app->navigators[navigator_index] = NULL;
    app->navigator_paths[navigator_index][0] = '\0';
    {
        int cooldown_index;
        int empty_index = -1;
        int oldest_index = 0;
        for (cooldown_index = 0; cooldown_index < NAVIGATOR_COUNT; ++cooldown_index) {
            if (app->navigator_cooldown_path[cooldown_index][0] == '\0') {
                if (empty_index < 0) empty_index = cooldown_index;
            } else if (strcmp(app->navigator_cooldown_path[cooldown_index], device_path) == 0) {
                empty_index = cooldown_index;
                break;
            } else if ((DWORD)(now - app->navigator_cooldown_tick[cooldown_index]) >
                       (DWORD)(now - app->navigator_cooldown_tick[oldest_index])) {
                oldest_index = cooldown_index;
            }
        }
        if (empty_index < 0) {
            empty_index = oldest_index;
        }
        strncpy(app->navigator_cooldown_path[empty_index], device_path, sizeof(app->navigator_cooldown_path[empty_index]) - 1);
        app->navigator_cooldown_path[empty_index][sizeof(app->navigator_cooldown_path[empty_index]) - 1] = '\0';
        app->navigator_cooldown_tick[empty_index] = now;
    }
    navigatorProfileResetRuntimeState(&app->navigator_profiles[navigator_index]);
    appContextZeroReport(app);
    vigemSubmitReport(app, &app->report);
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
        vigemSubmitReport(app, &app->report);
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
    int source_ids[MOVE_COUNT * 2];

    int newly_connected = 0;

    if (!app) return;

    count = psmove_count_connected(source_ids, MOVE_COUNT * 2);
    if (count <= 0) return;

    for (index = 0; index < count && newly_connected < 1; ++index) {
        PSMove *move = NULL;
        char *serial = NULL;
        int target_slot = -1;
        int sid = source_ids[index];
        int s;

        if (app->move_connected[0] && app->move_connected[1]) return;

        for (s = 0; s < MOVE_COUNT; ++s) {
            if (app->move_connected[s] && app->move_source_id[s] == sid) {
                sid = -1;
                break;
            }
        }
        if (sid < 0) continue;

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

        {
            int con_type = psmove_connection_type(move);
            bool keep = false;

            if (con_type == Conn_Bluetooth) {
                keep = true;
            } else {
                bool has_bt = false;
                for (s = 0; s < MOVE_COUNT; ++s) {
                    if (app->move_connected[s] && app->moves[s] &&
                        psmove_connection_type(app->moves[s]) == Conn_Bluetooth) {
                        has_bt = true;
                        break;
                    }
                }
                keep = !has_bt;
            }

            if (keep) {
                int test_ok = 0, test_bad = 0;
                while (test_ok + test_bad < 3 && psmove_poll(move)) {
                    int ax_t, ay_t, az_t;
                    psmove_get_accelerometer(move, &ax_t, &ay_t, &az_t);
                    if (ax_t >= -1000) ++test_ok; else ++test_bad;
                }
                keep = test_ok > 0;
            }

            if (!keep) {
                if (serial) psmove_free_mem(serial);
                psmove_disconnect(move);
                continue;
            }
        }

        app->moves[target_slot] = move;
        app->move_connected[target_slot] = true;
        ++newly_connected;
        app->move_source_id[target_slot] = source_ids[index];
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

void deviceConnectNavigatorsIfNeeded(AppContext *app, DWORD now)
{
    char device_path[NAVIGATOR_PATH_LENGTH];
    int device_count;
    int device_id;
    int navigator_index;

    if (!app) {
        return;
    }

    for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
        if (app->navigators[navigator_index] && !navDeviceIsConnected(app->navigators[navigator_index])) {
            deviceHandleNavigatorDisconnect(app, navigator_index, now);
        }
    }

    

    if (app->navigator_reconnect_tick != 0 &&
        (DWORD)(now - app->navigator_reconnect_tick) < 500) {
        return;
    }

    app->navigator_reconnect_tick = now;

    device_count = navDeviceGetAvailableCount();
    if (device_count <= 0) {
        return;
    }

    for (device_id = 0; device_id < device_count; ++device_id) {
        int target_slot = -1;

        if (!navDeviceGetIdentifier(device_id, device_path, (int)sizeof(device_path))) {
            continue;
        }

        if (isNavigatorPathAssigned(app, device_path)) {
            continue;
        }

        {
            int cooldown_index;
            for (cooldown_index = 0; cooldown_index < NAVIGATOR_COUNT; ++cooldown_index) {
                if (app->navigator_cooldown_path[cooldown_index][0] != '\0' &&
                    strcmp(app->navigator_cooldown_path[cooldown_index], device_path) == 0 &&
                    (DWORD)(now - app->navigator_cooldown_tick[cooldown_index]) < DEVICE_LIVENESS_POLL_MS) {
                    break;
                }
            }
            if (cooldown_index < NAVIGATOR_COUNT) {
                logWrite("navigator", "device %s on cooldown, skip", device_path);
                continue;
            }
        }

        if (!navDeviceIsReachable(device_path)) {
            logWrite("navigator", "device %s not reachable, skip", device_path);
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

        app->navigators[target_slot] = deviceConnectNavigatorByIdentifier(device_path);
        if (app->navigators[target_slot]) {
            navigatorProfileResetRuntimeState(&app->navigator_profiles[target_slot]);
            strncpy(app->navigator_paths[target_slot], device_path, sizeof(app->navigator_paths[target_slot]) - 1);
            app->navigator_paths[target_slot][sizeof(app->navigator_paths[target_slot]) - 1] = '\0';
            logWrite(
                "navigator",
                "Navigator%d connected: %s",
                target_slot + 1,
                app->navigator_paths[target_slot]
            );
        } else {
            logWrite(
                "navigator",
                "Navigator connect failed for id=%d",
                device_id
            );
        }
    }
}

void deviceCheckAlive(AppContext *app, DWORD now)
{
    int index;

    if (!app) return;

    for (index = 0; index < MOVE_COUNT; ++index) {
        if (app->moves[index]) {
            psmove_poll(app->moves[index]);
        }
    }

    (void)now;
}

bool deviceHasAnyInputConnected(const AppContext *app)
{
    int navigator_index;
    if (!app) return false;
    if (app->move_connected[0] || app->move_connected[1]) return true;
    if (app->move_count_cache > 0) return true;
    for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
        if (app->navigators[navigator_index]) return true;
    }
    return false;
}
