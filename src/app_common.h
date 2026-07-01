#ifndef APP_COMMON_H
#define APP_COMMON_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>
#include <wctype.h>
#include <math.h>
#include <string.h>
#include <stddef.h>

#ifndef _In_
#define _In_
#endif
#ifndef _In_opt_
#define _In_opt_
#endif
#ifndef _Out_
#define _Out_
#endif
#ifndef _Out_opt_
#define _Out_opt_
#endif
#ifndef _Inout_
#define _Inout_
#endif
#ifndef _Inout_opt_
#define _Inout_opt_
#endif
#ifndef _Outptr_
#define _Outptr_
#endif
#ifndef _Outptr_opt_
#define _Outptr_opt_
#endif
#ifndef _Function_class_
#define _Function_class_(x)
#endif
#ifndef _Must_inspect_result_
#define _Must_inspect_result_
#endif
#ifndef _Check_return_
#define _Check_return_
#endif
#ifndef _IRQL_requires_max_
#define _IRQL_requires_max_(x)
#endif
#ifndef _Return_type_success_
#define _Return_type_success_(x)
#endif
#ifndef FORCEINLINE
#define FORCEINLINE __inline
#endif
#ifndef VIGEM_DEPRECATED
#define VIGEM_DEPRECATED
#endif

#include "psmove.h"
#include "ViGEm/Client.h"

#define WM_TRAYICON               (WM_USER + 1)
#define ID_TRAY_TOGGLE_EMULATION  1000
#define ID_TRAY_CALIBRATE_PSMOVE  1001
#define ID_TRAY_EDIT_PROFILE      1003
#define ID_TRAY_EXIT              1002
#define ID_TRAY_PAIR_PSMOVE       1004
#define ID_TRAY_PROFILE_FIRST     1100
#define ID_TRAY_PROFILE_LAST      1227



#define WAIT_SLEEP_MS             250
#define SERVICE_POLL_MS           250
#define DEVICE_LIVENESS_POLL_MS   3000
#define MAX_PSMOVE_POLLS_PER_TICK 32
#define MOVE_COUNT                2
#define MOVE_SERIAL_LENGTH        18
#define NAVIGATOR_COUNT           2
#define NAVIGATOR_PATH_LENGTH     (MAX_PATH * 4)
#define MAX_ACTIONS_PER_INPUT     8
#define MAX_BOUND_INPUTS          16
#define MAX_SENSOR_ACTIONS        8
#define MAX_BIND_TEXT             256
#define INI_SECTION_SIZE          8192

#endif
