#include "logger.h"
#include <stdarg.h>

static HANDLE g_log_file = INVALID_HANDLE_VALUE;

static bool loggerBuildPath(wchar_t *out_path, size_t out_count)
{
    DWORD module_len;
    wchar_t *last_slash;
    if (!out_path || out_count == 0) return false;
    module_len = GetModuleFileNameW(NULL, out_path, (DWORD)out_count);
    if (module_len == 0 || module_len >= (DWORD)out_count) return false;
    last_slash = wcsrchr(out_path, L'\\');
    if (last_slash) {
        *(last_slash + 1) = L'\0';
        wcscat(out_path, L"motionpad.log");
    } else {
        wcsncpy(out_path, L"motionpad.log", out_count - 1);
        out_path[out_count - 1] = L'\0';
    }
    return true;
}

static void loggerWriteUtf8Text(const char *text)
{
    DWORD bytes_written;
    if (!text || g_log_file == INVALID_HANDLE_VALUE) return;
    WriteFile(g_log_file, text, (DWORD)strlen(text), &bytes_written, NULL);
}

static void loggerWriteWideText(const wchar_t *text)
{
    char utf8_buffer[4096];
    int utf8_len;
    DWORD bytes_written;
    if (!text || g_log_file == INVALID_HANDLE_VALUE) return;
    utf8_len = WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8_buffer, (int)sizeof(utf8_buffer), NULL, NULL);
    if (utf8_len <= 0) return;
    if (utf8_len > 1) {
        WriteFile(g_log_file, utf8_buffer, (DWORD)(utf8_len - 1), &bytes_written, NULL);
    }
}

static void loggerWritePrefix(const char *category)
{
    SYSTEMTIME st;
    char prefix[128];
    GetLocalTime(&st);
    _snprintf(prefix, sizeof(prefix), "[%02u.%02u.%02u %02u:%02u] | %s | ",
        (unsigned int)st.wDay,
        (unsigned int)st.wMonth,
        (unsigned int)(st.wYear % 100),
        (unsigned int)st.wHour,
        (unsigned int)st.wMinute,
        category ? category : "main");
    prefix[sizeof(prefix) - 1] = '\0';
    loggerWriteUtf8Text(prefix);
}

bool loggerInit(void)
{
    wchar_t log_path[MAX_PATH];
    loggerClose();
    if (!loggerBuildPath(log_path, MAX_PATH)) return false;
    g_log_file = CreateFileW(log_path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    return g_log_file != INVALID_HANDLE_VALUE;
}

void loggerClose(void)
{
    if (g_log_file != INVALID_HANDLE_VALUE) {
        CloseHandle(g_log_file);
        g_log_file = INVALID_HANDLE_VALUE;
    }
}

void loggerWriteA(const char *category, const char *fmt, ...)
{
    char message[2048];
    va_list args;
    if (g_log_file == INVALID_HANDLE_VALUE || !fmt) return;
    loggerWritePrefix(category);
    va_start(args, fmt);
    _vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    message[sizeof(message) - 1] = '\0';
    loggerWriteUtf8Text(message);
    loggerWriteUtf8Text("\r\n");
}

void loggerWriteW(const char *category, const wchar_t *fmt, ...)
{
    wchar_t message[2048];
    va_list args;
    if (g_log_file == INVALID_HANDLE_VALUE || !fmt) return;
    loggerWritePrefix(category);
    va_start(args, fmt);
    _vsnwprintf(message, sizeof(message) / sizeof(message[0]), fmt, args);
    va_end(args);
    message[(sizeof(message) / sizeof(message[0])) - 1] = L'\0';
    loggerWriteWideText(message);
    loggerWriteUtf8Text("\r\n");
}
