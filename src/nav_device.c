#include "nav_device.h"
#include "app_common.h"
#include "psnavigator.h"
#include "logger.h"
#include <SDL3/SDL.h>

#define NAV_DEVICE_GRACE_MS 1000
#define NAV_BATTERY_REFRESH_MS 30000

#define NAV_VID_SONY   0x054C
#define NAV_PID_NAV    0x042F

static NavBackend g_backend = NavBackend_LibNav;

static bool isSDL3NavigationDevice(SDL_JoystickID id)
{
    Uint16 vendor = SDL_GetJoystickVendorForID(id);
    Uint16 product = SDL_GetJoystickProductForID(id);
    return vendor == NAV_VID_SONY && product == NAV_PID_NAV;
}

void navDeviceGlobalInit(NavBackend backend)
{
    g_backend = backend;
    if (backend == NavBackend_SDL3) {
        if (!SDL_Init(SDL_INIT_JOYSTICK)) {
            g_backend = NavBackend_LibNav;
        }
    } else if (backend == NavBackend_LibNav) {
        psnavigatorInit(PSNAVIGATOR_API_VERSION);
    }
}

void navDeviceGlobalShutdown(void)
{
    if (g_backend == NavBackend_SDL3) {
        SDL_Quit();
    } else if (g_backend == NavBackend_LibNav) {
        psnavigatorShutdown();
    }
}

void navDeviceFrameStart(void)
{
    if (g_backend == NavBackend_SDL3) {
        SDL_PumpEvents();
    }
}

int navDeviceGetAvailableCount(void)
{
    if (g_backend == NavBackend_SDL3) {
        SDL_JoystickID *joysticks;
        int num_joysticks;
        int i;
        int count = 0;
        SDL_PumpEvents();
        joysticks = SDL_GetJoysticks(&num_joysticks);
        if (!joysticks) return 0;
        for (i = 0; i < num_joysticks; ++i) {
            if (isSDL3NavigationDevice(joysticks[i])) ++count;
        }
        SDL_free(joysticks);
        return count;
    }
    return psnavigatorGetDeviceCount();
}

bool navDeviceGetIdentifier(int index, char *buffer, int buffer_size)
{
    if (!buffer || buffer_size <= 0) return false;

    if (g_backend == NavBackend_SDL3) {
        SDL_JoystickID *joysticks;
        int num_joysticks;
        int i;
        int count = 0;
        SDL_PumpEvents();
        joysticks = SDL_GetJoysticks(&num_joysticks);
        if (!joysticks) return false;
        for (i = 0; i < num_joysticks; ++i) {
            if (isSDL3NavigationDevice(joysticks[i])) {
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
    return psnavigatorGetDevicePathById(index, buffer, buffer_size) == PSNAV_RESULT_OK;
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
    NavDeviceType type;
    bool connected;
    int connection_type;
    DWORD last_alive_tick;
    char last_error[256];
    union {
        PSNavigator *psnav;
        struct {
            SDL_Joystick *joy;
            SDL_JoystickID id;
            int num_buttons;
            int num_axes;
            int num_hats;
            bool buttons[32];
            Sint16 axes[8];
            Uint8 hats[4];
            DWORD battery_tick;
            int battery_level;
        } sdl3;
    } u;
};

bool navDeviceIsReachable(const char *identifier)
{
    if (!identifier || !identifier[0]) return false;

    if (g_backend == NavBackend_SDL3) {
        return true;
    }

    return true;
}

NavDevice *navDeviceCreate(const char *identifier)
{
    NavDevice *dev;

    if (!identifier || !identifier[0]) return NULL;

    dev = (NavDevice *)calloc(1, sizeof(NavDevice));
    if (!dev) return NULL;

    if (g_backend == NavBackend_SDL3) {
        int sdl_id;

        if (sscanf(identifier, "SDL3:%d", &sdl_id) != 1) {
            free(dev);
            return NULL;
        }

        dev->u.sdl3.joy = SDL_OpenJoystick((SDL_JoystickID)sdl_id);
        if (!dev->u.sdl3.joy) {
            strncpy(dev->last_error, SDL_GetError(), sizeof(dev->last_error) - 1);
            dev->last_error[sizeof(dev->last_error) - 1] = '\0';
            free(dev);
            return NULL;
        }

        dev->type = NavDevType_SDL3;
        dev->connected = true;
        dev->last_alive_tick = GetTickCount();
        dev->u.sdl3.battery_level = -1;
        dev->u.sdl3.id = (SDL_JoystickID)sdl_id;
        dev->u.sdl3.num_buttons = SDL_GetNumJoystickButtons(dev->u.sdl3.joy);
        dev->u.sdl3.num_axes = SDL_GetNumJoystickAxes(dev->u.sdl3.joy);
        dev->u.sdl3.num_hats = SDL_GetNumJoystickHats(dev->u.sdl3.joy);
        if (dev->u.sdl3.num_buttons > 32) dev->u.sdl3.num_buttons = 32;
        if (dev->u.sdl3.num_axes > 8) dev->u.sdl3.num_axes = 8;
        if (dev->u.sdl3.num_hats > 4) dev->u.sdl3.num_hats = 4;

        {
            SDL_JoystickConnectionState conn = SDL_GetJoystickConnectionState(dev->u.sdl3.joy);
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
            for (i = 0; i < dev->u.sdl3.num_buttons; ++i)
                dev->u.sdl3.buttons[i] = SDL_GetJoystickButton(dev->u.sdl3.joy, i);
            for (i = 0; i < dev->u.sdl3.num_axes; ++i)
                dev->u.sdl3.axes[i] = SDL_GetJoystickAxis(dev->u.sdl3.joy, i);
            for (i = 0; i < dev->u.sdl3.num_hats; ++i)
                dev->u.sdl3.hats[i] = SDL_GetJoystickHat(dev->u.sdl3.joy, i);
        }

        return dev;
    }

    {
        PSNavigator *psnav = psnavigatorConnectByPath(identifier);
        if (!psnav) {
            const char *err = psnavigatorGetLastError(NULL);
            if (err) {
                strncpy(dev->last_error, err, sizeof(dev->last_error) - 1);
                dev->last_error[sizeof(dev->last_error) - 1] = '\0';
            }
            free(dev);
            return NULL;
        }
        dev->type = NavDevType_LibNav;
        dev->connected = true;
        dev->last_alive_tick = GetTickCount();
        dev->connection_type = psnavigatorGetConnectionType(psnav);
        dev->u.psnav = psnav;
    }

    return dev;
}

void navDeviceDestroy(NavDevice *dev)
{
    if (!dev) return;

    if (dev->type == NavDevType_SDL3 && dev->u.sdl3.joy) {
        SDL_CloseJoystick(dev->u.sdl3.joy);
    } else if (dev->type == NavDevType_LibNav && dev->u.psnav) {
        psnavigatorDisconnect(dev->u.psnav);
    }

    dev->type = NavDevType_None;
    dev->connected = false;
    free(dev);
}

bool navDevicePoll(NavDevice *dev, DWORD now)
{
    if (!dev || !dev->connected) return false;

    if (dev->type == NavDevType_SDL3) {
        int i;
        if (!dev->u.sdl3.joy || !SDL_JoystickConnected(dev->u.sdl3.joy)) {
            if ((DWORD)(now - dev->last_alive_tick) >= NAV_DEVICE_GRACE_MS) {
                dev->connected = false;
            }
            return false;
        }
        for (i = 0; i < dev->u.sdl3.num_buttons; ++i)
            dev->u.sdl3.buttons[i] = SDL_GetJoystickButton(dev->u.sdl3.joy, i);
        for (i = 0; i < dev->u.sdl3.num_axes; ++i)
            dev->u.sdl3.axes[i] = SDL_GetJoystickAxis(dev->u.sdl3.joy, i);
        for (i = 0; i < dev->u.sdl3.num_hats; ++i)
            dev->u.sdl3.hats[i] = SDL_GetJoystickHat(dev->u.sdl3.joy, i);
        dev->last_alive_tick = now;
        return true;
    }

    if (dev->type == NavDevType_LibNav) {
        PSNavResult result = psnavigatorPoll(dev->u.psnav);
        if (result != PSNAV_RESULT_OK) {
            if (result == PSNAV_RESULT_NOT_CONNECTED &&
                (DWORD)(now - dev->last_alive_tick) >= NAV_DEVICE_GRACE_MS) {
                dev->connected = false;
            }
            return false;
        }
        dev->last_alive_tick = now;
        return true;
    }

    return false;
}

bool navDeviceIsConnected(NavDevice *dev)
{
    return dev && dev->connected;
}

bool navDeviceGetButton(NavDevice *dev, NavDeviceButton btn)
{
    if (!dev || !dev->connected || btn < 0 || btn >= NavDevBtn_COUNT) return false;

    if (dev->type == NavDevType_SDL3) {
        if (dev->u.sdl3.num_hats > 0) {
            switch (btn) {
                case NavDevBtn_UP:    return dev->u.sdl3.hats[0] & SDL_HAT_UP;
                case NavDevBtn_DOWN:  return dev->u.sdl3.hats[0] & SDL_HAT_DOWN;
                case NavDevBtn_LEFT:  return dev->u.sdl3.hats[0] & SDL_HAT_LEFT;
                case NavDevBtn_RIGHT: return dev->u.sdl3.hats[0] & SDL_HAT_RIGHT;
                default: break;
            }
        }
        int sdl_btn = g_sdl3_button_map[btn];
        if (sdl_btn < 0 || sdl_btn >= dev->u.sdl3.num_buttons) return false;
        return dev->u.sdl3.buttons[sdl_btn];
    }

    if (dev->type == NavDevType_LibNav) {
        static const PSNavButton btn_map[NavDevBtn_COUNT] = {
            PSNAV_BUTTON_L3,
            PSNAV_BUTTON_UP,
            PSNAV_BUTTON_RIGHT,
            PSNAV_BUTTON_DOWN,
            PSNAV_BUTTON_LEFT,
            PSNAV_BUTTON_L2,
            PSNAV_BUTTON_L1,
            PSNAV_BUTTON_CIRCLE,
            PSNAV_BUTTON_CROSS,
            PSNAV_BUTTON_PS
        };
        return psnavigatorGetButton(dev->u.psnav, btn_map[btn]) != 0;
    }

    return false;
}

uint32_t navDeviceGetButtons(NavDevice *dev)
{
    if (!dev || !dev->connected) return 0;

    if (dev->type == NavDevType_SDL3) {
        uint32_t mask = 0;
        int i;
        for (i = 0; i < dev->u.sdl3.num_buttons && i < 24; ++i) {
            if (dev->u.sdl3.buttons[i]) mask |= (1u << i);
        }
        return mask;
    }

    if (dev->type == NavDevType_LibNav) {
        return psnavigatorGetButtons(dev->u.psnav);
    }

    return 0;
}

int navDeviceGetAxis(NavDevice *dev, NavDeviceAxis axis)
{
    if (!dev || !dev->connected || axis < 0 || axis > NavDevAxis_TRIGGER) return 0;

    if (dev->type == NavDevType_SDL3) {
        int sdl_axis = g_sdl3_axis_map[axis];
        if (sdl_axis < 0 || sdl_axis >= dev->u.sdl3.num_axes) return 0;
        Sint16 val = dev->u.sdl3.axes[sdl_axis];
        if (axis == NavDevAxis_TRIGGER) {
            int normalized = (val + 32768) * 255 / 65535;
            if (normalized < 0) normalized = 0;
            if (normalized > 255) normalized = 255;
            return normalized;
        }
        return val / 256;
    }

    if (dev->type == NavDevType_LibNav) {
        static const PSNavAxis axis_map[] = {
            PSNAV_AXIS_STICK_X,
            PSNAV_AXIS_STICK_Y,
            PSNAV_AXIS_TRIGGER
        };
        return psnavigatorGetAxis(dev->u.psnav, axis_map[axis]);
    }

    return 0;
}

NavDeviceType navDeviceGetType(NavDevice *dev)
{
    if (!dev) return NavDevType_None;
    return dev->type;
}

static int sdl3BatteryFromHID(NavDevice *dev)
{
    const char *path = SDL_GetJoystickPath(dev->u.sdl3.joy);
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
    if (!dev || !dev->connected) return -1;

    if (dev->type == NavDevType_SDL3) {
        if (!dev->u.sdl3.joy) return -1;
        int percent;
        SDL_PowerState power = SDL_GetJoystickPowerInfo(dev->u.sdl3.joy, &percent);
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
            if (now - dev->u.sdl3.battery_tick >= NAV_BATTERY_REFRESH_MS) {
                int level = sdl3BatteryFromHID(dev);
                if (level != 0) {
                    dev->u.sdl3.battery_level = level;
                    dev->u.sdl3.battery_tick = now;
                } else if (dev->u.sdl3.battery_level < 0) {
                    dev->u.sdl3.battery_tick = now - NAV_BATTERY_REFRESH_MS + 3000;
                } else {
                    dev->u.sdl3.battery_level = level;
                    dev->u.sdl3.battery_tick = now;
                }
            }
            return dev->u.sdl3.battery_level;
        }
    }

    if (dev->type == NavDevType_LibNav) {
        return psnavigatorGetBattery(dev->u.psnav);
    }

    return -1;
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

    if (dev->type == NavDevType_LibNav && dev->u.psnav) {
        return psnavigatorGetLastError(dev->u.psnav);
    }

    return "no error";
}
