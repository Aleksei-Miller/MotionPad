#ifndef NAV_DEVICE_H
#define NAV_DEVICE_H

#include <stdbool.h>
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef enum NavDeviceType {
    NavDevType_None = 0,
    NavDevType_LibNav,
    NavDevType_SDL3
} NavDeviceType;

typedef enum NavBackend {
    NavBackend_LibNav = 0,
    NavBackend_SDL3
} NavBackend;

#define NAV_BATTERY_USB -2

typedef enum NavDeviceButton {
    NavDevBtn_L3 = 0,
    NavDevBtn_UP,
    NavDevBtn_RIGHT,
    NavDevBtn_DOWN,
    NavDevBtn_LEFT,
    NavDevBtn_L2,
    NavDevBtn_L1,
    NavDevBtn_CIRCLE,
    NavDevBtn_CROSS,
    NavDevBtn_PS,
    NavDevBtn_COUNT
} NavDeviceButton;

typedef enum NavDeviceAxis {
    NavDevAxis_STICK_X = 0,
    NavDevAxis_STICK_Y,
    NavDevAxis_TRIGGER
} NavDeviceAxis;

typedef struct NavDevice NavDevice;

void navDeviceGlobalInit(NavBackend backend);
void navDeviceGlobalShutdown(void);
int navDeviceGetAvailableCount(void);
bool navDeviceGetIdentifier(int index, char *buffer, int buffer_size);

bool navDeviceIsReachable(const char *identifier);
NavDevice *navDeviceCreate(const char *identifier);
void navDeviceDestroy(NavDevice *dev);

void navDeviceFrameStart(void);
bool navDevicePoll(NavDevice *dev, DWORD now);
bool navDeviceIsConnected(NavDevice *dev);
bool navDeviceGetButton(NavDevice *dev, NavDeviceButton btn);
uint32_t navDeviceGetButtons(NavDevice *dev);
int navDeviceGetAxis(NavDevice *dev, NavDeviceAxis axis);
NavDeviceType navDeviceGetType(NavDevice *dev);
int navDeviceGetBattery(NavDevice *dev);
int navDeviceGetConnectionType(NavDevice *dev);
const char *navDeviceGetLastError(NavDevice *dev);

#endif
