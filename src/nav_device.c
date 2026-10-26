#include "nav_device.h"
#include "app_common.h"
#include "logger.h"
#include <SDL3/SDL.h>

#define NAV_DEVICE_GRACE_MS 1000
#define NAV_BATTERY_REFRESH_MS 30000

#define NAV_VID_SONY   0x054C
#define NAV_PID_NAV    0x042F

static bool isNavigationDevice(SDL_JoystickID id)
{
    Uint16 vendor = SDL_GetJoystickVendorForID(id);
    Uint16 product = SDL_GetJoystickProductForID(id);
    return vendor == NAV_VID_SONY && product == NAV_PID_NAV;
}

void navDeviceGlobalInit(void)
{
    if (!SDL_Init(SDL_INIT_JOYSTICK)) {
        logWrite("nav", "SDL_Init(SDL_INIT_JOYSTICK) failed: %s", SDL_GetError());
    }
}

void navDeviceGlobalShutdown(void)
{
    SDL_Quit();
}

void navDeviceFrameStart(void)
{
    SDL_PumpEvents();
}

int navDeviceGetAvailableCount(void)
{
    SDL_JoystickID *joysticks;
    int num_joysticks;
    int i;
    int count = 0;
    SDL_PumpEvents();
    joysticks = SDL_GetJoysticks(&num_joysticks);
    if (!joysticks) return 0;
    for (i = 0; i < num_joysticks; ++i) {
        if (isNavigationDevice(joysticks[i])) ++count;
    }
    SDL_free(joysticks);
    return count;
}

bool navDeviceGetIdentifier(int index, char *buffer, int buffer_size)
{
    SDL_JoystickID *joysticks;
    int num_joysticks;
    int i;
    int count = 0;

    if (!buffer || buffer_size <= 0) return false;

    SDL_PumpEvents();
    joysticks = SDL_GetJoysticks(&num_joysticks);
    if (!joysticks) return false;
    for (i = 0; i < num_joysticks; ++i) {
        if (isNavigationDevice(joysticks[i])) {
            if (count == index) {
                _snprintf(buffer, buffer_size, "SDL3:%d", (int)joysticks[i]);
                buffer[buffer_size - 1] = '\0';
                SDL_free(joysticks);
                return true;
            }
            ++count;
        }
    }
    SDL_free(joysticks);
    return false;
}

static const int g_sdl3_button_map[NavDevBtn_COUNT] = {
    10,  /* NavDevBtn_L3 */
    4,   /* NavDevBtn_UP    */
    5,   /* NavDevBtn_RIGHT */
    6,   /* NavDevBtn_DOWN  */
    7,   /* NavDevBtn_LEFT  */
    4,   /* NavDevBtn_L2    */
    6,   /* NavDevBtn_L1    */
    1,   /* NavDevBtn_CIRCLE*/
    2,   /* NavDevBtn_CROSS */
    12,  /* NavDevBtn_PS    */
};

static const int g_sdl3_axis_map[3] = {
    0,  /* NavDevAxis_STICK_X */
    1,  /* NavDevAxis_STICK_Y */
    3,  /* NavDevAxis_TRIGGER */
};

struct NavDevice {
    bool connected;
    int connection_type;
    DWORD last_alive_tick;
    char last_error[256];
    SDL_Joystick *joy;
    int num_buttons;
    int num_axes;
    int num_hats;
    bool buttons[32];
    Sint16 axes[8];
    Uint8 hats[4];
    DWORD battery_tick;
    int battery_level;
};

bool navDeviceIsReachable(const char *identifier)
{
    return identifier && identifier[0];
}

NavDevice *navDeviceCreate(const char *identifier)
{
    NavDevice *dev;
    int sdl_id;

    if (!identifier || !identifier[0]) return NULL;

    dev = (NavDevice *)calloc(1, sizeof(NavDevice));
    if (!dev) return NULL;

    if (sscanf(identifier, "SDL3:%d", &sdl_id) != 1) {
        free(dev);
        return NULL;
    }

    dev->joy = SDL_OpenJoystick((SDL_JoystickID)sdl_id);
    if (!dev->joy) {
        strncpy(dev->last_error, SDL_GetError(), sizeof(dev->last_error) - 1);
        dev->last_error[sizeof(dev->last_error) - 1] = '\0';
        free(dev);
        return NULL;
    }

    dev->connected = true;
    dev->last_alive_tick = GetTickCount();
    dev->battery_level = -1;
    dev->num_buttons = SDL_GetNumJoystickButtons(dev->joy);
    dev->num_axes = SDL_GetNumJoystickAxes(dev->joy);
    dev->num_hats = SDL_GetNumJoystickHats(dev->joy);
    if (dev->num_buttons > 32) dev->num_buttons = 32;
    if (dev->num_axes > 8) dev->num_axes = 8;
    if (dev->num_hats > 4) dev->num_hats = 4;

    {
        SDL_JoystickConnectionState conn = SDL_GetJoystickConnectionState(dev->joy);
        if (conn == SDL_JOYSTICK_CONNECTION_WIRED)
            dev->connection_type = 1;
        else if (conn == SDL_JOYSTICK_CONNECTION_WIRELESS)
            dev->connection_type = 0;
        else
            dev->connection_type = 2;
    }

    SDL_PumpEvents();
    {
        int i;
        for (i = 0; i < dev->num_buttons; ++i)
            dev->buttons[i] = SDL_GetJoystickButton(dev->joy, i);
        for (i = 0; i < dev->num_axes; ++i)
            dev->axes[i] = SDL_GetJoystickAxis(dev->joy, i);
        for (i = 0; i < dev->num_hats; ++i)
            dev->hats[i] = SDL_GetJoystickHat(dev->joy, i);
    }

    return dev;
}

void navDeviceDestroy(NavDevice *dev)
{
    if (!dev) return;

    if (dev->joy) {
        SDL_CloseJoystick(dev->joy);
    }

    free(dev);
}

bool navDevicePoll(NavDevice *dev, DWORD now)
{
    int i;

    if (!dev || !dev->connected) return false;

    if (!dev->joy || !SDL_JoystickConnected(dev->joy)) {
        if ((DWORD)(now - dev->last_alive_tick) >= NAV_DEVICE_GRACE_MS) {
            dev->connected = false;
        }
        return false;
    }
    for (i = 0; i < dev->num_buttons; ++i)
        dev->buttons[i] = SDL_GetJoystickButton(dev->joy, i);
    for (i = 0; i < dev->num_axes; ++i)
        dev->axes[i] = SDL_GetJoystickAxis(dev->joy, i);
    for (i = 0; i < dev->num_hats; ++i)
        dev->hats[i] = SDL_GetJoystickHat(dev->joy, i);
    dev->last_alive_tick = now;
    return true;
}

bool navDeviceIsConnected(NavDevice *dev)
{
    return dev && dev->connected;
}

bool navDeviceGetButton(NavDevice *dev, NavDeviceButton btn)
{
    int sdl_btn;

    if (!dev || !dev->connected || btn < 0 || btn >= NavDevBtn_COUNT) return false;

    if (dev->num_hats > 0) {
        switch (btn) {
            case NavDevBtn_UP:    return dev->hats[0] & SDL_HAT_UP;
            case NavDevBtn_DOWN:  return dev->hats[0] & SDL_HAT_DOWN;
            case NavDevBtn_LEFT:  return dev->hats[0] & SDL_HAT_LEFT;
            case NavDevBtn_RIGHT: return dev->hats[0] & SDL_HAT_RIGHT;
            default: break;
        }
    }
    sdl_btn = g_sdl3_button_map[btn];
    if (sdl_btn < 0 || sdl_btn >= dev->num_buttons) return false;
    return dev->buttons[sdl_btn];
}

uint32_t navDeviceGetButtons(NavDevice *dev)
{
    uint32_t mask = 0;
    int i;

    if (!dev || !dev->connected) return 0;

    for (i = 0; i < dev->num_buttons && i < 24; ++i) {
        if (dev->buttons[i]) mask |= (1u << i);
    }
    return mask;
}

int navDeviceGetAxis(NavDevice *dev, NavDeviceAxis axis)
{
    int sdl_axis;
    Sint16 val;

    if (!dev || !dev->connected || axis < 0 || axis > NavDevAxis_TRIGGER) return 0;

    sdl_axis = g_sdl3_axis_map[axis];
    if (sdl_axis < 0 || sdl_axis >= dev->num_axes) return 0;
    val = dev->axes[sdl_axis];
    if (axis == NavDevAxis_TRIGGER) {
        int normalized = (val + 32768) * 255 / 65535;
        if (normalized < 0) normalized = 0;
        if (normalized > 255) normalized = 255;
        return normalized;
    }
    return val / 256;
}

static int navBatteryFromHID(NavDevice *dev)
{
    const char *path = SDL_GetJoystickPath(dev->joy);
    if (!path) return -1;

    wchar_t wpath[512];
    if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, 512) <= 0)
        return -1;

    HANDLE h = CreateFileW(wpath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;

    int result = -1;
    HMODULE hhid = LoadLibraryA("hid.dll");
    if (!hhid) { CloseHandle(h); return -1; }

    typedef BOOLEAN (WINAPI *HidD_GetFeature_t)(HANDLE, PVOID, ULONG);
    HidD_GetFeature_t fn = (HidD_GetFeature_t)(void*)GetProcAddress(hhid, "HidD_GetFeature");
    if (fn) {
        unsigned char feat[64];
        memset(feat, 0, sizeof(feat));
        if (fn(h, feat, 50)) {
            int bat = feat[31];
            switch (bat) {
                case 0x05: result = 100; break;
                case 0x04: result = 80; break;
                case 0x03: result = 60; break;
                case 0x02: result = 40; break;
                case 0x01: result = 20; break;
                case 0x00: result = 0; break;
                case 0xEE:
                case 0xEF: result = NAV_BATTERY_USB; break;
            }
        }
    }
    FreeLibrary(hhid);
    CloseHandle(h);
    return result;
}

int navDeviceGetBattery(NavDevice *dev)
{
    int percent;

    if (!dev || !dev->connected) return -1;

    if (!dev->joy) return -1;
    SDL_PowerState power = SDL_GetJoystickPowerInfo(dev->joy, &percent);
    if (power == SDL_POWERSTATE_CHARGING ||
        power == SDL_POWERSTATE_CHARGED ||
        power == SDL_POWERSTATE_NO_BATTERY) {
        return 100;
    }
    if (power == SDL_POWERSTATE_ON_BATTERY && percent > 0) {
        return percent;
    }
    {
        DWORD now = GetTickCount();
        if (now - dev->battery_tick >= NAV_BATTERY_REFRESH_MS) {
            int level = navBatteryFromHID(dev);
            if (level != 0) {
                dev->battery_level = level;
                dev->battery_tick = now;
            } else if (dev->battery_level < 0) {
                dev->battery_tick = now - NAV_BATTERY_REFRESH_MS + 3000;
            } else {
                dev->battery_level = level;
                dev->battery_tick = now;
            }
        }
        return dev->battery_level;
    }
}

int navDeviceGetConnectionType(NavDevice *dev)
{
    if (!dev || !dev->connected) return 2;
    return dev->connection_type;
}

const char *navDeviceGetLastError(NavDevice *dev)
{
    if (!dev) return "device is NULL";
    if (dev->last_error[0]) return dev->last_error;

    return "no error";
}
