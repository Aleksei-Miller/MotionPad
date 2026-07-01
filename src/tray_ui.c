#ifdef __TINYC__
#ifndef PROCESS_QUERY_LIMITED_INFORMATION
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000
#endif
#endif

#include "tray_ui.h"
#include "rumble_manager.h"
#include "vigem_manager.h"
#include "logger.h"
#include "nav_device.h"
#include "profile.h"
#include "profile_watcher.h"

#ifdef __TINYC__
typedef BOOL (WINAPI *QFPIWFunc)(HANDLE, DWORD, LPWSTR, PDWORD);
static QFPIWFunc qfpiw = NULL;
#endif

static AppContext *g_app = NULL;

#define MAX_TRAY_PROFILES (ID_TRAY_PROFILE_LAST - ID_TRAY_PROFILE_FIRST + 1)

typedef struct TrayProfileEntry {
    wchar_t path[MAX_PATH];
    wchar_t relative_path[MAX_PATH];
    wchar_t label[128];
    wchar_t exe_path[MAX_PATH];
} TrayProfileEntry;

static TrayProfileEntry g_tray_profiles[MAX_TRAY_PROFILES];
static UINT g_tray_profile_count = 0;

static void logLastErrorW(const wchar_t *prefix)
{
    DWORD error_code = GetLastError();
    wchar_t *message_buffer = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&message_buffer, 0, NULL);
    if (message_buffer) {
        logWriteW("winapi", L"%ls: %ls (code=%lu)", prefix, message_buffer, error_code);
        LocalFree(message_buffer);
    } else {
        logWriteW("winapi", L"%ls: code=%lu", prefix, error_code);
    }
}

static int navigatorBatteryRawToPercent(int battery_raw, bool is_sdl3)
{
    if (is_sdl3) {
        return battery_raw;
    }
    switch (battery_raw) {
        case 0x00: return 0;
        case 0x01: return 20;
        case 0x02: return 40;
        case 0x03: return 60;
        case 0x04: return 80;
        case 0x05: return 100;
        case 0xEE: return 100;
        case 0xEF: return 100;
        default: return 0;
    }
}

static bool buildExecutableDirectory(wchar_t *out_dir, size_t out_count)
{
    DWORD module_len;
    wchar_t *last_slash;

    if (!out_dir || out_count == 0) return false;

    module_len = GetModuleFileNameW(NULL, out_dir, (DWORD)out_count);
    if (module_len == 0 || module_len >= (DWORD)out_count) {
        logLastErrorW(L"GetModuleFileNameW failed");
        return false;
    }

    last_slash = wcsrchr(out_dir, L'\\');
    if (!last_slash) {
        logWrite("tray", "failed to determine executable directory");
        return false;
    }

    *last_slash = L'\0';
    return true;
}

static bool buildProfilesDirectory(wchar_t *out_dir, size_t out_count)
{
    if (!buildExecutableDirectory(out_dir, out_count)) {
        return false;
    }

    if (wcslen(out_dir) + wcslen(L"\\profiles") + 1 >= out_count) {
        logWrite("tray", "profiles directory path is too long");
        return false;
    }

    wcscat(out_dir, L"\\profiles");
    return true;
}

static void normalizePathForCompare(const wchar_t *path, wchar_t *out_path, size_t out_count)
{
    size_t in_index = 0;
    size_t out_index = 0;
    bool previous_was_slash = false;

    if (!out_path || out_count == 0) {
        return;
    }

    out_path[0] = L'\0';
    if (!path) {
        return;
    }

    while (path[in_index] != L'\0' && out_index + 1 < out_count) {
        wchar_t ch = path[in_index++];
        bool is_slash = (ch == L'\\' || ch == L'/');

        if (is_slash) {
            if (previous_was_slash) {
                continue;
            }
            ch = L'\\';
            previous_was_slash = true;
        } else {
            previous_was_slash = false;
        }

        out_path[out_index++] = (wchar_t)towlower(ch);
    }

    out_path[out_index] = L'\0';
}

static bool profilePathsEqual(const wchar_t *left, const wchar_t *right)
{
    wchar_t left_normalized[MAX_PATH];
    wchar_t right_normalized[MAX_PATH];

    normalizePathForCompare(left, left_normalized, MAX_PATH);
    normalizePathForCompare(right, right_normalized, MAX_PATH);
    return wcscmp(left_normalized, right_normalized) == 0;
}

static void getCurrentRelativeProfilePath(const AppContext *app, wchar_t *out_path, size_t out_count)
{
    const wchar_t *file_name;

    if (!out_path || out_count == 0) {
        return;
    }

    out_path[0] = L'\0';
    if (!app || app->config_path[0] == L'\0') {
        return;
    }

    file_name = wcsrchr(app->config_path, L'\\');
    file_name = file_name ? file_name + 1 : app->config_path;
    _snwprintf(out_path, out_count, L"profiles\\%ls", file_name);
    out_path[out_count - 1] = L'\0';
}

static void getFileNameWithoutExtension(const wchar_t *path, wchar_t *out_name, size_t out_count)
{
    const wchar_t *file_name;
    const wchar_t *dot;
    size_t copy_len;

    if (!out_name || out_count == 0) {
        return;
    }

    out_name[0] = L'\0';
    if (!path || path[0] == L'\0') {
        return;
    }

    file_name = wcsrchr(path, L'\\');
    file_name = file_name ? file_name + 1 : path;
    dot = wcsrchr(file_name, L'.');
    copy_len = dot && dot > file_name ? (size_t)(dot - file_name) : wcslen(file_name);
    if (copy_len >= out_count) {
        copy_len = out_count - 1;
    }

    wcsncpy(out_name, file_name, copy_len);
    out_name[copy_len] = L'\0';
}

static void loadProfileMenuLabel(const wchar_t *profile_path, wchar_t *out_label, size_t out_count)
{
    if (!out_label || out_count == 0) {
        return;
    }

    out_label[0] = L'\0';
    if (!profile_path || profile_path[0] == L'\0') {
        return;
    }

    GetPrivateProfileStringW(L"Profile", L"Name", L"", out_label, (DWORD)out_count, profile_path);
    trimWideStringInPlace(out_label);
    if (out_label[0] == L'\0') {
        getFileNameWithoutExtension(profile_path, out_label, out_count);
    }
}

static int getProfileSortRank(const wchar_t *relative_path)
{
    if (_wcsicmp(relative_path, L"profiles\\hybrid mode.ini") == 0) return 0;
    if (_wcsicmp(relative_path, L"profiles\\controller mode.ini") == 0) return 1;
    if (_wcsicmp(relative_path, L"profiles\\arrow mode.ini") == 0) return 2;
    if (_wcsicmp(relative_path, L"profiles\\wasd mode.ini") == 0) return 3;
    return 4;
}

static int compareTrayProfiles(const void *left, const void *right)
{
    const TrayProfileEntry *left_entry = (const TrayProfileEntry *)left;
    const TrayProfileEntry *right_entry = (const TrayProfileEntry *)right;
    int rank_diff;
    int name_cmp;

    rank_diff = getProfileSortRank(left_entry->relative_path) - getProfileSortRank(right_entry->relative_path);
    if (rank_diff != 0) {
        return rank_diff;
    }

    name_cmp = _wcsicmp(left_entry->label, right_entry->label);
    if (name_cmp != 0) {
        return name_cmp;
    }

    return _wcsicmp(left_entry->path, right_entry->path);
}

static void refreshTrayProfiles(void)
{
    wchar_t profiles_dir[MAX_PATH];
    wchar_t search_mask[MAX_PATH];
    WIN32_FIND_DATAW find_data;
    HANDLE find_handle;

    g_tray_profile_count = 0;

    if (!buildProfilesDirectory(profiles_dir, MAX_PATH)) {
        return;
    }

    if (_snwprintf(search_mask, MAX_PATH, L"%ls\\*.ini", profiles_dir) < 0) {
        return;
    }
    search_mask[MAX_PATH - 1] = L'\0';

    find_handle = FindFirstFileW(search_mask, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        TrayProfileEntry *entry;

        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            continue;
        }
        if (g_tray_profile_count >= MAX_TRAY_PROFILES) {
            break;
        }

        entry = &g_tray_profiles[g_tray_profile_count];
        if (_snwprintf(entry->path, MAX_PATH, L"%ls\\%ls", profiles_dir, find_data.cFileName) < 0) {
            continue;
        }
        entry->path[MAX_PATH - 1] = L'\0';
        if (_snwprintf(entry->relative_path, MAX_PATH, L"profiles\\%ls", find_data.cFileName) < 0) {
            continue;
        }
        entry->relative_path[MAX_PATH - 1] = L'\0';
        loadProfileMenuLabel(entry->path, entry->label, sizeof(entry->label) / sizeof(entry->label[0]));
        entry->exe_path[0] = L'\0';
        GetPrivateProfileStringW(L"Profile", L"Path", L"", entry->exe_path, MAX_PATH, entry->path);
        trimWideStringInPlace(entry->exe_path);
        ++g_tray_profile_count;
    } while (FindNextFileW(find_handle, &find_data));

    FindClose(find_handle);

    if (g_tray_profile_count > 1) {
        qsort(g_tray_profiles, g_tray_profile_count, sizeof(g_tray_profiles[0]), compareTrayProfiles);
    }
}

static void appendProfilesMenu(HMENU menu_handle, const AppContext *app)
{
    HMENU profiles_menu;
    UINT profile_index;
    wchar_t current_relative_path[MAX_PATH];

    if (!menu_handle) {
        return;
    }

    refreshTrayProfiles();
    getCurrentRelativeProfilePath(app, current_relative_path, MAX_PATH);
    profiles_menu = CreatePopupMenu();
    if (!profiles_menu) {
        return;
    }

    if (g_tray_profile_count == 0) {
        AppendMenuW(profiles_menu, MF_STRING | MF_GRAYED, 0, L"No profiles");
    } else {
        for (profile_index = 0; profile_index < g_tray_profile_count; ++profile_index) {
            UINT menu_flags = MF_STRING;

            if (app && profilePathsEqual(current_relative_path, g_tray_profiles[profile_index].relative_path)) {
                menu_flags |= MF_CHECKED;
            }

            AppendMenuW(
                profiles_menu,
                menu_flags,
                ID_TRAY_PROFILE_FIRST + profile_index,
                g_tray_profiles[profile_index].label
            );

            if (profile_index + 1 < g_tray_profile_count) {
                int current_rank = getProfileSortRank(g_tray_profiles[profile_index].relative_path);
                int next_rank = getProfileSortRank(g_tray_profiles[profile_index + 1].relative_path);
                if (current_rank != next_rank && (current_rank == 1 || current_rank == 3)) {
                    AppendMenuW(profiles_menu, MF_SEPARATOR, 0, NULL);
                }
            }
        }
    }

    AppendMenuW(menu_handle, MF_POPUP, (UINT_PTR)profiles_menu, L"Profile");
}

static void traySelectProfile(AppContext *app, UINT command_id)
{
    const TrayProfileEntry *selected_profile;

    if (!app || command_id < ID_TRAY_PROFILE_FIRST) {
        return;
    }

    command_id -= ID_TRAY_PROFILE_FIRST;
    if (command_id >= g_tray_profile_count) {
        return;
    }

    selected_profile = &g_tray_profiles[command_id];
    if (profilePathsEqual(app->config_path, selected_profile->path)) {
        return;
    }

    app->auto_triggered_exe[0] = L'\0';
    wcsncpy(app->pending_profile_path, selected_profile->path, MAX_PATH - 1);
    app->pending_profile_path[MAX_PATH - 1] = L'\0';
    InterlockedExchange(&app->profile_switch_pending, 1);
    logWriteW("tray", L"profile queued: %ls (%ls)", selected_profile->label, selected_profile->path);
}

void trayApplyPendingProfileSelection(AppContext *app)
{
    if (!app) {
        return;
    }

    if (!InterlockedExchange(&app->profile_switch_pending, 0)) {
        return;
    }

    if (app->pending_profile_path[0] == L'\0' ||
        profilePathsEqual(app->config_path, app->pending_profile_path)) {
        app->pending_profile_path[0] = L'\0';
        return;
    }

    profileWatcherStop(app);
    appContextResetInputsKeepPad(app);

    wcsncpy(app->config_path, app->pending_profile_path, MAX_PATH - 1);
    app->config_path[MAX_PATH - 1] = L'\0';
    app->pending_profile_path[0] = L'\0';

    profileSaveSettings(app);
    profileLoadAll(app);
    InterlockedExchange(&app->config_dirty, 0);

    if (!profileWatcherStart(app)) {
        logWriteW("config", L"failed to start watcher for: %ls", app->config_path);
    }

    logWriteW("tray", L"profile applied: %ls", app->config_path);
    trayUpdateTooltip(app);
}

static void getForegroundProcessName(wchar_t *out_name, size_t out_count)
{
    HWND foreground;
    DWORD pid;
    HANDLE process;
    DWORD size;
    wchar_t full_path[MAX_PATH];
    wchar_t *slash;

    if (!out_name || out_count == 0) return;
    out_name[0] = L'\0';

    foreground = GetForegroundWindow();
    if (!foreground) return;

    if (!GetWindowThreadProcessId(foreground, &pid)) return;
    if (pid == 0) return;

    process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return;

    size = MAX_PATH;
    full_path[0] = L'\0';

#if defined(__TINYC__)
    if (!qfpiw) {
        HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
        if (k32) qfpiw = (QFPIWFunc)GetProcAddress(k32, "QueryFullProcessImageNameW");
    }
    if (qfpiw && qfpiw(process, 0, full_path, &size) && full_path[0] != L'\0') {
#else
    if (QueryFullProcessImageNameW(process, 0, full_path, &size) && full_path[0] != L'\0') {
#endif
        slash = wcsrchr(full_path, L'\\');
        if (slash) {
            wcsncpy(out_name, slash + 1, out_count - 1);
            out_name[out_count - 1] = L'\0';
        } else {
            wcsncpy(out_name, full_path, out_count - 1);
            out_name[out_count - 1] = L'\0';
        }
    }
    CloseHandle(process);
}

static bool profileExeMatches(const wchar_t *profile_exe_path, const wchar_t *foreground_exe)
{
    const wchar_t *slash;

    if (!profile_exe_path || profile_exe_path[0] == L'\0' || !foreground_exe) return false;

    slash = wcsrchr(profile_exe_path, L'\\');
    if (slash) {
        return _wcsicmp(foreground_exe, slash + 1) == 0;
    }
    return _wcsicmp(foreground_exe, profile_exe_path) == 0;
}

void trayCheckAutoProfile(AppContext *app, DWORD now)
{
    static DWORD last_check = 0;
    wchar_t exe_name[MAX_PATH];
    UINT i;

    if (!app || !app->use_auto_profile) return;
    if ((DWORD)(now - last_check) < 200) return;
    last_check = now;
    if (InterlockedCompareExchange(&app->profile_switch_pending, 0, 0)) return;

    if (g_tray_profile_count == 0) {
        refreshTrayProfiles();
    }

    getForegroundProcessName(exe_name, MAX_PATH);
    if (exe_name[0] == L'\0') return;

    for (i = 0; i < g_tray_profile_count; ++i) {
        if (g_tray_profiles[i].exe_path[0] == L'\0') continue;
        if (!profileExeMatches(g_tray_profiles[i].exe_path, exe_name)) continue;
        if (profilePathsEqual(app->config_path, g_tray_profiles[i].path)) {
            wcsncpy(app->auto_triggered_exe, exe_name, MAX_PATH - 1);
            app->auto_triggered_exe[MAX_PATH - 1] = L'\0';
            return;
        }

        if (app->auto_triggered_exe[0] == L'\0') {
            wcsncpy(app->auto_base_path, app->config_path, MAX_PATH - 1);
            app->auto_base_path[MAX_PATH - 1] = L'\0';
        }
        wcsncpy(app->auto_triggered_exe, exe_name, MAX_PATH - 1);
        app->auto_triggered_exe[MAX_PATH - 1] = L'\0';

        wcsncpy(app->pending_profile_path, g_tray_profiles[i].path, MAX_PATH - 1);
        app->pending_profile_path[MAX_PATH - 1] = L'\0';
        InterlockedExchange(&app->profile_switch_pending, 1);
        logWriteW("tray", L"auto-profile: window=%ls (%ls)", exe_name, g_tray_profiles[i].label);
        return;
    }

    if (app->auto_triggered_exe[0] != L'\0' &&
        _wcsicmp(exe_name, app->auto_triggered_exe) != 0 &&
        !profilePathsEqual(app->config_path, app->auto_base_path)) {
        wcsncpy(app->pending_profile_path, app->auto_base_path, MAX_PATH - 1);
        app->pending_profile_path[MAX_PATH - 1] = L'\0';
        app->auto_triggered_exe[0] = L'\0';
        InterlockedExchange(&app->profile_switch_pending, 1);
        logWriteW("tray", L"auto-profile: revert to base");
    }
}

static void buildBatteryMenuText(const AppContext *app, wchar_t *out_text, size_t out_count)
{
    int navigator_index;
    size_t pos = 0;
    int move_count = 0;
    int navigator_count = 0;

    if (!app || !out_text || out_count == 0) return;

    out_text[0] = L'\0';

    if (app->moves[0]) ++move_count;
    if (app->moves[1]) ++move_count;
    for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
        if (app->navigators[navigator_index]) ++navigator_count;
    }
    if (app->moves[0]) {
        int raw = app->move_battery_raw[0];
        if (raw == Batt_CHARGING || raw == Batt_CHARGING_DONE) {
            pos += (size_t)_snwprintf(
                out_text + pos, out_count - pos,
                (move_count == 1) ? L"Move: USB" : L"Move 1: USB"
            );
        } else if (raw < 0) {
            bool is_usb = psmove_connection_type(app->moves[0]) == Conn_USB;
            pos += (size_t)_snwprintf(
                out_text + pos, out_count - pos,
                is_usb ? ((move_count == 1) ? L"Move: USB" : L"Move 1: USB")
                       : ((move_count == 1) ? L"Move" : L"Move 1")
            );
        } else {
            int pct;
            switch (raw) {
                case Batt_MIN: pct = 0; break;
                case Batt_20Percent: pct = 20; break;
                case Batt_40Percent: pct = 40; break;
                case Batt_60Percent: pct = 60; break;
                case Batt_80Percent: pct = 80; break;
                case Batt_MAX: pct = 100; break;
                default: pct = 0; break;
            }
            pos += (size_t)_snwprintf(
                out_text + pos, out_count - pos,
                (move_count == 1) ? L"Move: %d%%" : L"Move 1: %d%%",
                pct
            );
        }
    }

    if (pos < out_count && app->moves[1]) {
        int raw = app->move_battery_raw[1];
        if (raw == Batt_CHARGING || raw == Batt_CHARGING_DONE) {
            pos += (size_t)_snwprintf(
                out_text + pos, out_count - pos, L" | Move 2: USB"
            );
        } else if (raw < 0) {
            pos += (size_t)_snwprintf(
                out_text + pos, out_count - pos,
                psmove_connection_type(app->moves[1]) == Conn_USB ? L" | Move 2: USB" : L" | Move 2"
            );
        } else {
            int pct;
            switch (raw) {
                case Batt_MIN: pct = 0; break;
                case Batt_20Percent: pct = 20; break;
                case Batt_40Percent: pct = 40; break;
                case Batt_60Percent: pct = 60; break;
                case Batt_80Percent: pct = 80; break;
                case Batt_MAX: pct = 100; break;
                default: pct = 0; break;
            }
            pos += (size_t)_snwprintf(
                out_text + pos, out_count - pos, L" | Move 2: %d%%",
                pct
            );
        }
    }

    for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT && pos < out_count; ++navigator_index) {
        if (app->navigators[navigator_index]) {
            bool is_sdl3 = navDeviceGetType(app->navigators[navigator_index]) == NavDevType_SDL3;
            int battery_raw = navDeviceGetBattery(app->navigators[navigator_index]);
            int percent = -1;

            if (battery_raw == NAV_BATTERY_USB) {
                percent = NAV_BATTERY_USB;
            } else if (battery_raw >= 0) {
                percent = navigatorBatteryRawToPercent(battery_raw, is_sdl3);
            }

            if (percent == NAV_BATTERY_USB) {
                pos += (size_t)_snwprintf(
                    out_text + pos,
                    out_count - pos,
                    pos ? L" | Nav %d: USB" : L"Nav %d: USB",
                    navigator_index + 1
                );
            } else if (percent < 0) {
                pos += (size_t)_snwprintf(
                    out_text + pos,
                    out_count - pos,
                    pos ? L" | Nav %d" : L"Nav %d",
                    navigator_index + 1
                );
            } else {
                pos += (size_t)_snwprintf(
                    out_text + pos,
                    out_count - pos,
                    pos ? L" | Nav %d: %d%%" : L"Nav %d: %d%%",
                    navigator_index + 1,
                    percent
                );
            }
        }
    }

    if (out_text[0] == L'\0') {
        _snwprintf(out_text, out_count, L"No controllers");
    }

    out_text[out_count - 1] = L'\0';
}

void trayUpdateTooltip(AppContext *app)
{
    wchar_t tooltip_text[128];

    if (!app || !app->tray_added) return;

    buildBatteryMenuText(app, tooltip_text, 128);
    wcsncpy(app->tray_icon.szTip, tooltip_text, (sizeof(app->tray_icon.szTip) / sizeof(app->tray_icon.szTip[0])) - 1);
    app->tray_icon.szTip[(sizeof(app->tray_icon.szTip) / sizeof(app->tray_icon.szTip[0])) - 1] = L'\0';
    app->tray_icon.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &app->tray_icon);
}

void trayToggleEmulation(AppContext *app)
{
    if (!app) return;
    app->output_enabled = !app->output_enabled;
    profileSaveSettings(app);
    if (!app->output_enabled) {
        logWrite("main", "emulation disabled");
        appContextStopEmulation(app);
    } else {
        logWrite("main", "emulation enabled");
    }
    trayUpdateTooltip(app);
}

static void trayLaunchPSMoveCalibration(void)
{
    wchar_t app_dir[MAX_PATH];
    wchar_t command_line[MAX_PATH];
    STARTUPINFOW startup_info;
    PROCESS_INFORMATION process_info;
    int written;

    if (!buildExecutableDirectory(app_dir, MAX_PATH)) return;

    written = _snwprintf(command_line, MAX_PATH, L"\"%ls\\psmove.exe\" calibrate", app_dir);
    if (written < 0 || written >= MAX_PATH) {
        logWrite("tray", "psmove.exe path is too long");
        return;
    }
    command_line[MAX_PATH - 1] = L'\0';

    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    ZeroMemory(&process_info, sizeof(process_info));

    if (!CreateProcessW(NULL, command_line, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, app_dir, &startup_info, &process_info)) {
        logLastErrorW(L"CreateProcessW(psmove.exe) failed");
        return;
    }

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
    logWrite("tray", "started psmove.exe");
}

static void trayPairPSMove(void)
{
    wchar_t app_dir[MAX_PATH];
    wchar_t exe_path[MAX_PATH];
    // Show instruction to the user
    MessageBoxW(NULL, L"Please connect PS Move via USB to PC.", L"Pair PS Move", MB_OK | MB_ICONINFORMATION);

    if (!buildExecutableDirectory(app_dir, MAX_PATH)) return;

    if (_snwprintf(exe_path, MAX_PATH, L"%ls\\psmove.exe", app_dir) < 0) {
        logWrite("tray", "psmove.exe path is too long");
        return;
    }
    exe_path[MAX_PATH - 1] = L'\0';

    // Launch psmove.exe with "pair" argument using elevated privileges
    HINSTANCE result = ShellExecuteW(NULL, L"runas", exe_path, L"pair", NULL, SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32) {
        logLastErrorW(L"ShellExecuteW(psmove.exe pair) failed");
    } else {
        logWrite("tray", "started psmove.exe for pairing");
    }
}

static void trayEditProfile(AppContext *app)
{
    HINSTANCE result;

    if (!app || !app->config_path || app->config_path[0] == L'\0') {
        logWrite("tray", "no profile to edit");
        return;
    }

    result = ShellExecuteW(NULL, L"open", app->config_path, NULL, NULL, SW_SHOW);
    if ((INT_PTR)result <= 32) {
        logWriteW("tray", L"ShellExecuteW failed: %p", result);
        return;
    }

    logWriteW("tray", L"opened profile: %ls", app->config_path);
}

static LRESULT CALLBACK trayWindowProc(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message) {
        case WM_TRAYICON:
            if (l_param == WM_RBUTTONUP || l_param == WM_CONTEXTMENU) {
                HMENU menu_handle = CreatePopupMenu();
                if (menu_handle && g_app) {
                    POINT cursor_pos;
                    UINT toggle_flags = MF_STRING | (g_app->output_enabled ? MF_UNCHECKED : MF_CHECKED);
                    wchar_t battery_text[64];
                    buildBatteryMenuText(g_app, battery_text, 64);
                    AppendMenuW(menu_handle, MF_STRING | MF_GRAYED, 0, battery_text);
                    AppendMenuW(menu_handle, MF_SEPARATOR, 0, NULL);
					AppendMenuW(menu_handle, MF_STRING, ID_TRAY_EDIT_PROFILE, L"Edit Profile");
                    appendProfilesMenu(menu_handle, g_app);
                    AppendMenuW(menu_handle, MF_SEPARATOR, 0, NULL);
                    AppendMenuW(menu_handle, toggle_flags, ID_TRAY_TOGGLE_EMULATION, L"Disable");
                    HMENU service_menu = CreatePopupMenu();
                    if (service_menu) {
                        AppendMenuW(service_menu, MF_STRING, ID_TRAY_CALIBRATE_PSMOVE, L"Calibrate PS Move");
                        AppendMenuW(service_menu, MF_STRING, ID_TRAY_PAIR_PSMOVE, L"Pair PS Move");
                        AppendMenuW(menu_handle, MF_POPUP, (UINT_PTR)service_menu, L"Service");
                     }
                    AppendMenuW(menu_handle, MF_SEPARATOR, 0, NULL);
                    AppendMenuW(menu_handle, MF_STRING, ID_TRAY_EXIT, L"Exit");
                    GetCursorPos(&cursor_pos);
                    SetForegroundWindow(window_handle);
                    TrackPopupMenu(menu_handle, TPM_RIGHTBUTTON, cursor_pos.x, cursor_pos.y, 0, window_handle, NULL);
                    DestroyMenu(menu_handle);
                }
                return 0;
            }
            break;
        case WM_COMMAND:
            if (LOWORD(w_param) >= ID_TRAY_PROFILE_FIRST && LOWORD(w_param) <= ID_TRAY_PROFILE_LAST) {
                traySelectProfile(g_app, LOWORD(w_param));
                return 0;
            }
            if (LOWORD(w_param) == ID_TRAY_TOGGLE_EMULATION) {
                trayToggleEmulation(g_app);
                return 0;
            }
            if (LOWORD(w_param) == ID_TRAY_PAIR_PSMOVE) {
                trayPairPSMove();
                return 0;
            }
            if (LOWORD(w_param) == ID_TRAY_CALIBRATE_PSMOVE) {
                trayLaunchPSMoveCalibration();
                return 0;
            }
            if (LOWORD(w_param) == ID_TRAY_EDIT_PROFILE) {
                trayEditProfile(g_app);
                return 0;
            }
            if (LOWORD(w_param) == ID_TRAY_EXIT) {
                PostQuitMessage(0);
                return 0;
            }
            break;
        case WM_CLOSE:
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(window_handle, message, w_param, l_param);
}

bool trayInitWindow(AppContext *app)
{
    WNDCLASSW window_class;
    if (!app) return false;
    g_app = app;
    ZeroMemory(&window_class, sizeof(window_class));
    window_class.lpfnWndProc = trayWindowProc;
    window_class.hInstance = GetModuleHandleW(NULL);
    window_class.lpszClassName = L"PSMoveLiveGestureWindowClass";
    if (!RegisterClassW(&window_class)) {
        DWORD error_code = GetLastError();
        if (error_code != ERROR_CLASS_ALREADY_EXISTS) {
            logLastErrorW(L"RegisterClassW failed");
            return false;
        }
    }
    app->window = CreateWindowW(window_class.lpszClassName, L"PSMoveLiveGesture", 0, 0, 0, 0, 0,
        NULL, NULL, window_class.hInstance, NULL);
    if (!app->window) {
        logLastErrorW(L"CreateWindowW failed");
        return false;
    }
    return true;
}

bool trayInitIcon(AppContext *app)
{
    wchar_t tooltip_text[128];
    HINSTANCE instance_handle;

    if (!app || !app->window) return false;
    ZeroMemory(&app->tray_icon, sizeof(app->tray_icon));
    app->tray_icon.cbSize = sizeof(app->tray_icon);
    app->tray_icon.hWnd = app->window;
    app->tray_icon.uID = 1;
    app->tray_icon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    app->tray_icon.uCallbackMessage = WM_TRAYICON;
    instance_handle = GetModuleHandleW(NULL);
    app->tray_icon.hIcon = (HICON)LoadImageW(
        instance_handle,
        L"IDI_APP_ICON",
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR
    );
    if (!app->tray_icon.hIcon) {
        logLastErrorW(L"LoadImageW(IDI_APP_ICON) failed");
        app->tray_icon.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    buildBatteryMenuText(app, tooltip_text, 128);
    wcsncpy(app->tray_icon.szTip, tooltip_text, (sizeof(app->tray_icon.szTip) / sizeof(app->tray_icon.szTip[0])) - 1);
    app->tray_icon.szTip[(sizeof(app->tray_icon.szTip) / sizeof(app->tray_icon.szTip[0])) - 1] = L'\0';
    if (!Shell_NotifyIconW(NIM_ADD, &app->tray_icon)) {
        logLastErrorW(L"Shell_NotifyIconW(NIM_ADD) failed");
        return false;
    }
    app->tray_added = true;
    return true;
}

void trayRemoveIcon(AppContext *app)
{
    if (!app) return;
    if (app->tray_icon.cbSize != 0) {
        Shell_NotifyIconW(NIM_DELETE, &app->tray_icon);
        ZeroMemory(&app->tray_icon, sizeof(app->tray_icon));
    }
    app->tray_added = false;
}

void trayShutdownWindow(AppContext *app)
{
    if (!app) return;
    if (app->window) {
        DestroyWindow(app->window);
        app->window = NULL;
    }
    if (g_app == app) g_app = NULL;
}

void trayHandleMessageLoop(AppContext *app)
{
    MSG message;
    if (!app) return;
    while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            InterlockedExchange(&app->running, 0);
            break;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}
