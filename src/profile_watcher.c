#include "profile_watcher.h"
#include "logger.h"

static void copyFileNameFromPath(const wchar_t *path, wchar_t *out_name, size_t out_count)
{
    const wchar_t *last_slash;

    if (!out_name || out_count == 0) {
        return;
    }

    out_name[0] = L'\0';

    if (!path || path[0] == L'\0') {
        return;
    }

    last_slash = wcsrchr(path, L'\\');
    if (last_slash) {
        ++last_slash;
    } else {
        last_slash = path;
    }

    wcsncpy(out_name, last_slash, out_count - 1);
    out_name[out_count - 1] = L'\0';
}

static bool copyDirectoryFromFilePath(const wchar_t *path, wchar_t *out_dir, size_t out_count)
{
    wchar_t *last_slash;

    if (!path || !out_dir || out_count == 0) {
        return false;
    }

    wcsncpy(out_dir, path, out_count - 1);
    out_dir[out_count - 1] = L'\0';

    last_slash = wcsrchr(out_dir, L'\\');
    if (!last_slash) {
        return false;
    }

    *last_slash = L'\0';
    return true;
}

static bool fileNameEqualsNotifyName(const wchar_t *expected_name, const FILE_NOTIFY_INFORMATION *info)
{
    size_t notify_len;

    if (!expected_name || !info) {
        return false;
    }

    notify_len = (size_t)(info->FileNameLength / sizeof(WCHAR));
    if (wcslen(expected_name) != notify_len) {
        return false;
    }

    return _wcsnicmp(expected_name, info->FileName, notify_len) == 0;
}

static DWORD WINAPI profileWatcherThreadProc(LPVOID param)
{
    AppContext *app = (AppContext *)param;
    wchar_t directory_path[MAX_PATH];
    wchar_t file_name[MAX_PATH];
    HANDLE directory_handle;
    BYTE buffer[2048];
    OVERLAPPED overlapped;
    DWORD bytes_returned;

    if (!app) {
        return 0;
    }

    if (!copyDirectoryFromFilePath(app->config_path, directory_path, MAX_PATH)) {
        logWriteW("config", L"watcher: failed to resolve directory from path: %ls", app->config_path);
        return 0;
    }

    copyFileNameFromPath(app->config_path, file_name, MAX_PATH);

    directory_handle = CreateFileW(
        directory_path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (directory_handle == INVALID_HANDLE_VALUE) {
        logWriteW("config", L"watcher: CreateFileW failed for directory: %ls", directory_path);
        return 0;
    }

    logWriteW("config", L"watcher started: %ls", app->config_path);

    ZeroMemory(&overlapped, sizeof(overlapped));
    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!overlapped.hEvent) {
        CloseHandle(directory_handle);
        logWrite("config", "watcher: CreateEventW failed");
        return 0;
    }

    for (;;) {
        HANDLE wait_handles[2];
        DWORD wait_result;
        FILE_NOTIFY_INFORMATION *info;
        bool matched = false;

        ResetEvent(overlapped.hEvent);
        ZeroMemory(buffer, sizeof(buffer));
        bytes_returned = 0;

        if (!ReadDirectoryChangesW(
                directory_handle,
                buffer,
                sizeof(buffer),
                FALSE,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
                &bytes_returned,
                &overlapped,
                NULL)) {
            DWORD error_code = GetLastError();
            if (error_code == ERROR_OPERATION_ABORTED) {
                break;
            }
            logWrite("config", "watcher: ReadDirectoryChangesW failed: %lu", (unsigned long)error_code);
            break;
        }

        wait_handles[0] = app->config_watch_stop_event;
        wait_handles[1] = overlapped.hEvent;
        wait_result = WaitForMultipleObjects(2, wait_handles, FALSE, INFINITE);

        if (wait_result == WAIT_OBJECT_0) {
            CancelIo(directory_handle);
            break;
        }

        if (wait_result != WAIT_OBJECT_0 + 1) {
            logWrite("config", "watcher: WaitForMultipleObjects failed: %lu", (unsigned long)GetLastError());
            CancelIo(directory_handle);
            break;
        }

        if (!GetOverlappedResult(directory_handle, &overlapped, &bytes_returned, FALSE)) {
            DWORD error_code = GetLastError();
            if (error_code == ERROR_OPERATION_ABORTED) {
                break;
            }
            logWrite("config", "watcher: GetOverlappedResult failed: %lu", (unsigned long)error_code);
            break;
        }

        info = (FILE_NOTIFY_INFORMATION *)buffer;
        for (;;) {
            if (fileNameEqualsNotifyName(file_name, info)) {
                matched = true;
                break;
            }

            if (info->NextEntryOffset == 0) {
                break;
            }
            info = (FILE_NOTIFY_INFORMATION *)((BYTE *)info + info->NextEntryOffset);
        }

        if (matched) {
            Sleep(100);
            InterlockedExchange(&app->config_dirty, 1);
        }
    }

    CloseHandle(overlapped.hEvent);
    CloseHandle(directory_handle);
    logWrite("config", "watcher stopped");
    return 0;
}

static DWORD WINAPI settingsWatcherThreadProc(LPVOID param)
{
    AppContext *app = (AppContext *)param;
    wchar_t directory_path[MAX_PATH];
    wchar_t file_name[MAX_PATH];
    HANDLE directory_handle;
    BYTE buffer[2048];
    OVERLAPPED overlapped;
    DWORD bytes_returned;

    if (!app) {
        return 0;
    }

    if (!copyDirectoryFromFilePath(app->settings_path, directory_path, MAX_PATH)) {
        logWriteW("settings", L"watcher: failed to resolve directory from path: %ls", app->settings_path);
        return 0;
    }

    copyFileNameFromPath(app->settings_path, file_name, MAX_PATH);

    directory_handle = CreateFileW(
        directory_path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (directory_handle == INVALID_HANDLE_VALUE) {
        logWriteW("settings", L"watcher: CreateFileW failed for directory: %ls", directory_path);
        return 0;
    }

    logWriteW("settings", L"watcher started: %ls", app->settings_path);

    ZeroMemory(&overlapped, sizeof(overlapped));
    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!overlapped.hEvent) {
        CloseHandle(directory_handle);
        logWrite("settings", "watcher: CreateEventW failed");
        return 0;
    }

    for (;;) {
        HANDLE wait_handles[2];
        DWORD wait_result;
        FILE_NOTIFY_INFORMATION *info;
        bool matched = false;

        ResetEvent(overlapped.hEvent);
        ZeroMemory(buffer, sizeof(buffer));
        bytes_returned = 0;

        if (!ReadDirectoryChangesW(
                directory_handle,
                buffer,
                sizeof(buffer),
                FALSE,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
                &bytes_returned,
                &overlapped,
                NULL)) {
            DWORD error_code = GetLastError();
            if (error_code == ERROR_OPERATION_ABORTED) {
                break;
            }
            logWrite("settings", "watcher: ReadDirectoryChangesW failed: %lu", (unsigned long)error_code);
            break;
        }

        wait_handles[0] = app->settings_watch_stop_event;
        wait_handles[1] = overlapped.hEvent;
        wait_result = WaitForMultipleObjects(2, wait_handles, FALSE, INFINITE);

        if (wait_result == WAIT_OBJECT_0) {
            CancelIo(directory_handle);
            break;
        }

        if (wait_result != WAIT_OBJECT_0 + 1) {
            logWrite("settings", "watcher: WaitForMultipleObjects failed: %lu", (unsigned long)GetLastError());
            CancelIo(directory_handle);
            break;
        }

        if (!GetOverlappedResult(directory_handle, &overlapped, &bytes_returned, FALSE)) {
            DWORD error_code = GetLastError();
            if (error_code == ERROR_OPERATION_ABORTED) {
                break;
            }
            logWrite("settings", "watcher: GetOverlappedResult failed: %lu", (unsigned long)error_code);
            break;
        }

        info = (FILE_NOTIFY_INFORMATION *)buffer;
        for (;;) {
            if (fileNameEqualsNotifyName(file_name, info)) {
                matched = true;
                break;
            }

            if (info->NextEntryOffset == 0) {
                break;
            }
            info = (FILE_NOTIFY_INFORMATION *)((BYTE *)info + info->NextEntryOffset);
        }

        if (matched) {
            Sleep(100);
            InterlockedExchange(&app->settings_dirty, 1);
        }
    }

    CloseHandle(overlapped.hEvent);
    CloseHandle(directory_handle);
    logWrite("settings", "watcher stopped");
    return 0;
}

bool settingsWatcherStart(AppContext *app)
{
    DWORD thread_id;

    if (!app) {
        return false;
    }

    if (app->settings_watch_thread) {
        return true;
    }

    app->settings_watch_stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!app->settings_watch_stop_event) {
        logWrite("settings", "watcher: failed to create stop event");
        return false;
    }

    app->settings_watch_thread = CreateThread(
        NULL,
        0,
        settingsWatcherThreadProc,
        app,
        0,
        &thread_id
    );

    if (!app->settings_watch_thread) {
        CloseHandle(app->settings_watch_stop_event);
        app->settings_watch_stop_event = NULL;
        logWrite("settings", "watcher: failed to create thread");
        return false;
    }

    return true;
}

void settingsWatcherStop(AppContext *app)
{
    if (!app) {
        return;
    }

    if (app->settings_watch_stop_event) {
        SetEvent(app->settings_watch_stop_event);
    }

    if (app->settings_watch_thread) {
        WaitForSingleObject(app->settings_watch_thread, 2000);
        CloseHandle(app->settings_watch_thread);
        app->settings_watch_thread = NULL;
    }

    if (app->settings_watch_stop_event) {
        CloseHandle(app->settings_watch_stop_event);
        app->settings_watch_stop_event = NULL;
    }
}

bool profileWatcherStart(AppContext *app)
{
    DWORD thread_id;

    if (!app) {
        return false;
    }

    if (app->config_watch_thread) {
        return true;
    }

    app->config_watch_stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!app->config_watch_stop_event) {
        logWrite("config", "watcher: failed to create stop event");
        return false;
    }

    app->config_watch_thread = CreateThread(
        NULL,
        0,
        profileWatcherThreadProc,
        app,
        0,
        &thread_id
    );

    if (!app->config_watch_thread) {
        CloseHandle(app->config_watch_stop_event);
        app->config_watch_stop_event = NULL;
        logWrite("config", "watcher: failed to create thread");
        return false;
    }

    return true;
}

void profileWatcherStop(AppContext *app)
{
    if (!app) {
        return;
    }

    if (app->config_watch_stop_event) {
        SetEvent(app->config_watch_stop_event);
    }

    if (app->config_watch_thread) {
        WaitForSingleObject(app->config_watch_thread, 2000);
        CloseHandle(app->config_watch_thread);
        app->config_watch_thread = NULL;
    }

    if (app->config_watch_stop_event) {
        CloseHandle(app->config_watch_stop_event);
        app->config_watch_stop_event = NULL;
    }
}
