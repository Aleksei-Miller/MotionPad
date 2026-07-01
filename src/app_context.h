#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "app_common.h"
#include "move_input.h"
#include "navigator_input.h"
#include "nav_device.h"

typedef struct RumbleMotorConfig {
    int min_active;
    int low_threshold;
    int mid_threshold;
    float boost_low;
    float boost_mid;
    float boost_high;
    int min_output;
} RumbleMotorConfig;

typedef struct RumbleConfig {
    int enabled;
    int combine_when_single_move;
    float master_strength;
    RumbleMotorConfig large;
    RumbleMotorConfig small;
} RumbleConfig;

typedef struct AppContext {
    HWND window;
    NOTIFYICONDATAW tray_icon;
    wchar_t settings_path[MAX_PATH];
    wchar_t config_path[MAX_PATH];
    wchar_t pending_profile_path[MAX_PATH];
    volatile LONG config_dirty;
    volatile LONG profile_switch_pending;
    wchar_t auto_base_path[MAX_PATH];
    wchar_t auto_triggered_exe[MAX_PATH];
    HANDLE config_watch_thread;
    HANDLE config_watch_stop_event;
    volatile LONG settings_dirty;
    HANDLE settings_watch_thread;
    HANDLE settings_watch_stop_event;

    PVIGEM_CLIENT vigem_client;
    PVIGEM_TARGET vigem_pad;
    bool vigem_client_ready;
    bool vigem_notification_registered;

    volatile LONG running;
    bool output_enabled;
    bool tray_added;

    MoveProfile move_profiles[MOVE_COUNT];
    PSMove *moves[MOVE_COUNT];
    bool move_connected[MOVE_COUNT];
    int move_source_id[MOVE_COUNT];
    char move_serials[MOVE_COUNT][MOVE_SERIAL_LENGTH];

    int move_battery_raw[MOVE_COUNT];

    NavDevice *navigators[NAVIGATOR_COUNT];
    NavigatorProfile navigator_profiles[NAVIGATOR_COUNT];
    char navigator_paths[NAVIGATOR_COUNT][NAVIGATOR_PATH_LENGTH];
    char navigator_cooldown_path[NAVIGATOR_COUNT][NAVIGATOR_PATH_LENGTH];
    DWORD navigator_cooldown_tick[NAVIGATOR_COUNT];
    DWORD navigator_reconnect_tick;
    bool use_bthps3;
    bool use_auto_profile;
    int poll_rate_ms;
    RumbleConfig rumble_config;
    BOOL telemetry_enabled;
    XUSB_REPORT report;
    int move_count_cache;
} AppContext;

void appContextInit(AppContext *app);
void appContextReleaseSyntheticInputs(AppContext *app);
void appContextZeroReport(AppContext *app);
void appContextResetInputsKeepPad(AppContext *app);
void appContextStopEmulation(AppContext *app);

#endif
