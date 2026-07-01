#include "telemetry.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <winerror.h>

static HANDLE g_pipe = INVALID_HANDLE_VALUE;
static HANDLE g_thread = NULL;
static volatile LONG g_running = 0;
static CRITICAL_SECTION g_cs;
static volatile LONG g_connected = 0;

static DWORD WINAPI PipeServerThread(LPVOID lpParam)
{
    (void)lpParam;

    while (InterlockedCompareExchange(&g_running, 1, 1)) {
if (g_pipe != INVALID_HANDLE_VALUE) {
            DWORD avail = 0;
            if (!PeekNamedPipe(g_pipe, NULL, 0, NULL, &avail, NULL)) {
                DWORD err = GetLastError();
                logWrite("telemetry", "Pipe disconnected, err=%lu", err);
                CloseHandle(g_pipe);
                g_pipe = INVALID_HANDLE_VALUE;
                InterlockedExchange(&g_connected, 0);
            }
        } else {
            g_pipe = CreateNamedPipeW(
                TELEMETRY_PIPE_NAME,
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_BYTE | PIPE_WAIT,
                1,
                4096,
                4096,
                100,
                NULL
            );

            if (g_pipe == INVALID_HANDLE_VALUE) {
                DWORD err = GetLastError();
                logWrite("telemetry", "CreateNamedPipe failed, err=%lu", err);
                Sleep(100);
                continue;
            }

            if (ConnectNamedPipe(g_pipe, NULL)) {
                logWrite("telemetry", "Client connected");
                InterlockedExchange(&g_connected, 1);
            } else {
                DWORD err = GetLastError();
                logWrite("telemetry", "ConnectNamedPipe failed, err=%lu", err);
                CloseHandle(g_pipe);
                g_pipe = INVALID_HANDLE_VALUE;
                Sleep(100);
            }
        }
        Sleep(10);
    }

    if (g_pipe != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(g_pipe);
        CloseHandle(g_pipe);
        g_pipe = INVALID_HANDLE_VALUE;
    }

    return 0;
}

void telemetryInit(void)
{
    InitializeCriticalSection(&g_cs);
    g_running = 1;
    g_pipe = INVALID_HANDLE_VALUE;
    g_connected = 0;

    logWrite("telemetry", "Starting pipe server on \\\\.\\pipe\\motionpad");
    g_thread = CreateThread(NULL, 0, PipeServerThread, NULL, 0, NULL);
}

void telemetryShutdown(void)
{
    if (g_running) {
        InterlockedExchange(&g_running, 0);
    }

    if (g_thread) {
        WaitForSingleObject(g_thread, 1000);
        CloseHandle(g_thread);
        g_thread = NULL;
    }

    DeleteCriticalSection(&g_cs);
}

void telemetrySend(const TelemetryData *data)
{
    if (!data || InterlockedCompareExchange(&g_connected, 0, 0) == 0) {
        return;
    }

    char json[512];
    int len = snprintf(json, sizeof(json),
        "{\"t\":%lu,"
        "\"m1\":{\"con\":%d,\"bat\":%d,\"btns\":%d,\"trig\":%d,\"gyro\":[%d,%d,%d],\"accel\":[%d,%d,%d]},"
        "\"m2\":{\"con\":%d,\"bat\":%d,\"btns\":%d,\"trig\":%d,\"gyro\":[%d,%d,%d],\"accel\":[%d,%d,%d]},"
        "\"n1\":{\"con\":%d,\"bat\":%d,\"btns\":%d,\"trig\":%d,\"x\":%d,\"y\":%d},"
        "\"n2\":{\"con\":%d,\"bat\":%d,\"btns\":%d,\"trig\":%d,\"x\":%d,\"y\":%d}}\n",
        data->tick,
        data->m1.con, data->m1.bat, data->m1.btns, data->m1.trig, data->m1.gyro[0], data->m1.gyro[1], data->m1.gyro[2], data->m1.accel[0], data->m1.accel[1], data->m1.accel[2],
        data->m2.con, data->m2.bat, data->m2.btns, data->m2.trig, data->m2.gyro[0], data->m2.gyro[1], data->m2.gyro[2], data->m2.accel[0], data->m2.accel[1], data->m2.accel[2],
        data->n1.con, data->n1.bat, data->n1.btns, data->n1.trig, data->n1.x, data->n1.y,
        data->n2.con, data->n2.bat, data->n2.btns, data->n2.trig, data->n2.x, data->n2.y
    );

    if (len <= 0) return;

    EnterCriticalSection(&g_cs);

    DWORD written = 0;
    DWORD avail = 0;
    if (PeekNamedPipe(g_pipe, NULL, 0, NULL, &avail, NULL)) {
        BOOL ok = WriteFile(g_pipe, json, len, &written, NULL);
        (void)ok;
    }

    LeaveCriticalSection(&g_cs);
}

BOOL telemetryIsConnected(void)
{
    return InterlockedCompareExchange(&g_connected, 0, 0);
}
