#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <windows.h>

#define TELEMETRY_PIPE_NAME L"\\\\.\\pipe\\motionpad"

typedef struct {
    int con;
    int bat;
    int btns;
    int trig;
    int gyro[3];
    int accel[3];
} MoveData;

typedef struct {
    int con;
    int bat;
    int btns;
    int trig;
    int x;
    int y;
} NavigatorData;

typedef struct {
    DWORD tick;
    MoveData m1;
    MoveData m2;
    NavigatorData n1;
    NavigatorData n2;
} TelemetryData;

void telemetryInit(void);
void telemetryShutdown(void);
void telemetrySend(const TelemetryData *data);
BOOL telemetryIsConnected(void);

#endif
