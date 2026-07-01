#include "profile.h"
#include "logger.h"

void trimWideStringInPlace(wchar_t *str)
{
    wchar_t *end;
    wchar_t *start;

    if (!str || !*str)
        return;

    // trim start
    start = str;
    while (*start == L' ' || *start == L'\t' || *start == L'\r' || *start == L'\n')
        start++;

    if (start != str)
        memmove(str, start, (wcslen(start) + 1) * sizeof(wchar_t));

    // trim end
    end = str + wcslen(str) - 1;
    while (end >= str && (*end == L' ' || *end == L'\t' || *end == L'\r' || *end == L'\n'))
    {
        *end = L'\0';
        end--;
    }
}

static void sanitizeRumbleMotorConfig(RumbleMotorConfig *cfg, int default_min_output)
{
    if (!cfg) return;
    if (cfg->min_active < 0) cfg->min_active = 0;
    if (cfg->low_threshold < 0) cfg->low_threshold = 0;
    if (cfg->mid_threshold < cfg->low_threshold) cfg->mid_threshold = cfg->low_threshold;
    if (cfg->boost_low < 0.0f) cfg->boost_low = 0.0f;
    if (cfg->boost_mid < 0.0f) cfg->boost_mid = 0.0f;
    if (cfg->boost_high < 0.0f) cfg->boost_high = 0.0f;
    if (cfg->min_output < 0) cfg->min_output = 0;
    if (cfg->min_output > 255) cfg->min_output = 255;
    if (cfg->min_output == 0) cfg->min_output = default_min_output;
}

float profileReadFloat(const wchar_t *config_path, const wchar_t *section, const wchar_t *key, const wchar_t *default_value)
{
    wchar_t value_text[64];
    ZeroMemory(value_text, sizeof(value_text));
    GetPrivateProfileStringW(section, key, default_value, value_text, 64, config_path);
    return (float)wcstod(value_text, NULL);
}

AxisSettings profileReadAxisSettings(const wchar_t *config_path, const wchar_t *section_name, const wchar_t *axis_suffix, int default_invert, const wchar_t *default_sensitivity, int default_deadzone)
{
    AxisSettings axis;
    wchar_t key_name[64];

    _snwprintf(key_name, 64, L"Invert%ls", axis_suffix);
    key_name[63] = L'\0';
    axis.invert = GetPrivateProfileIntW(section_name, key_name, default_invert, config_path) ? 1 : 0;

    _snwprintf(key_name, 64, L"Sensitivity%ls", axis_suffix);
    key_name[63] = L'\0';
    axis.sensitivity = profileReadFloat(config_path, section_name, key_name, default_sensitivity);
    if (axis.sensitivity < 0.0f) axis.sensitivity = 0.0f;

    _snwprintf(key_name, 64, L"Deadzone%ls", axis_suffix);
    key_name[63] = L'\0';
    axis.deadzone = GetPrivateProfileIntW(section_name, key_name, default_deadzone, config_path);
    if (axis.deadzone < 0) axis.deadzone = 0;

    return axis;
}

int profileReadKeyboardKey(const wchar_t *config_path)
{
    int repeat_ms = GetPrivateProfileIntW(L"Keyboard", L"Key", 120, config_path);
    if (repeat_ms < 1) repeat_ms = 1;
    return repeat_ms;
}

int profileReadMouseKey(const wchar_t *config_path)
{
    int repeat_ms = GetPrivateProfileIntW(L"Mouse", L"Key", 120, config_path);
    if (repeat_ms < 1) repeat_ms = 1;
    return repeat_ms;
}

int profileReadMouseWheelMs(const wchar_t *config_path)
{
    int repeat_ms = GetPrivateProfileIntW(L"Mouse", L"Wheel", 0, config_path);
    if (repeat_ms < 0) repeat_ms = 0;
    return repeat_ms;
}

static void makeMoveSectionName(wchar_t *out_name, size_t out_count, int move_index)
{
    _snwprintf(out_name, out_count, L"Move%d", move_index + 1);
    out_name[out_count - 1] = L'\0';
}

static void makeAltMoveSectionName(wchar_t *out_name, size_t out_count, int move_index)
{
    _snwprintf(out_name, out_count, L"AltMove%d", move_index + 1);
    out_name[out_count - 1] = L'\0';
}

static void makeNavigationSectionName(wchar_t *out_name, size_t out_count, int navigator_index)
{
    _snwprintf(out_name, out_count, L"Navigation%d", navigator_index + 1);
    out_name[out_count - 1] = L'\0';
}

static void makeNavigationStickSectionName(wchar_t *out_name, size_t out_count, int navigator_index)
{
    _snwprintf(out_name, out_count, L"NavigationStick%d", navigator_index + 1);
    out_name[out_count - 1] = L'\0';
}

static void makeMoveTriggerSectionName(wchar_t *out_name, size_t out_count, int move_index)
{
    _snwprintf(out_name, out_count, L"MoveTrigger%d", move_index + 1);
    out_name[out_count - 1] = L'\0';
}

static void makeAltMoveTriggerSectionName(wchar_t *out_name, size_t out_count, int move_index)
{
    _snwprintf(out_name, out_count, L"AltMoveTrigger%d", move_index + 1);
    out_name[out_count - 1] = L'\0';
}

static void makeNavigationTriggerSectionName(wchar_t *out_name, size_t out_count, int navigator_index)
{
    _snwprintf(out_name, out_count, L"NavigationTrigger%d", navigator_index + 1);
    out_name[out_count - 1] = L'\0';
}

static void makeAltNavigationTriggerSectionName(wchar_t *out_name, size_t out_count, int navigator_index)
{
    _snwprintf(out_name, out_count, L"AltNavigationTrigger%d", navigator_index + 1);
    out_name[out_count - 1] = L'\0';
}

static void makeAltNavigationSectionName(wchar_t *out_name, size_t out_count, int navigator_index)
{
    _snwprintf(out_name, out_count, L"AltNavigation%d", navigator_index + 1);
    out_name[out_count - 1] = L'\0';
}

static void makeAltNavigationStickSectionName(wchar_t *out_name, size_t out_count, int navigator_index)
{
    _snwprintf(out_name, out_count, L"AltNavigationStick%d", navigator_index + 1);
    out_name[out_count - 1] = L'\0';
}

static void makeControllerSensorSectionName(wchar_t *out_name, size_t out_count, const wchar_t *base_name, int move_index)
{
    _snwprintf(out_name, out_count, L"%ls%d", base_name, move_index + 1);
    out_name[out_count - 1] = L'\0';
}

static void profileReadGyroConfig(const wchar_t *config_path, int move_index, AxisSettings config[SensorAxisIndex_Count])
{
    wchar_t section_name[64];
    makeControllerSensorSectionName(section_name, 64, L"MoveGyroscope", move_index);
    config[SensorAxisIndex_X] = profileReadAxisSettings(config_path, section_name, L"X", 0, L"1.0", 80);
    config[SensorAxisIndex_Y] = profileReadAxisSettings(config_path, section_name, L"Y", 1, L"1.0", 80);
    config[SensorAxisIndex_Z] = profileReadAxisSettings(config_path, section_name, L"Z", 0, L"1.0", 150);
}

static void profileReadAccelConfig(const wchar_t *config_path, int move_index, AxisSettings config[SensorAxisIndex_Count])
{
    wchar_t section_name[64];
    makeControllerSensorSectionName(section_name, 64, L"MoveAccelerometer", move_index);
    config[SensorAxisIndex_X] = profileReadAxisSettings(config_path, section_name, L"X", 0, L"1.0", 300);
    config[SensorAxisIndex_Y] = profileReadAxisSettings(config_path, section_name, L"Y", 0, L"1.0", 300);
    config[SensorAxisIndex_Z] = profileReadAxisSettings(config_path, section_name, L"Z", 0, L"1.0", 500);
}

static void profileReadAltGyroConfig(const wchar_t *config_path, int move_index, AxisSettings config[SensorAxisIndex_Count])
{
    wchar_t section_name[64];
    makeControllerSensorSectionName(section_name, 64, L"AltMoveGyroscope", move_index);
    config[SensorAxisIndex_X] = profileReadAxisSettings(config_path, section_name, L"X", 0, L"1.0", 80);
    config[SensorAxisIndex_Y] = profileReadAxisSettings(config_path, section_name, L"Y", 1, L"1.0", 80);
    config[SensorAxisIndex_Z] = profileReadAxisSettings(config_path, section_name, L"Z", 0, L"1.0", 150);
}

static void profileReadAltAccelConfig(const wchar_t *config_path, int move_index, AxisSettings config[SensorAxisIndex_Count])
{
    wchar_t section_name[64];
    makeControllerSensorSectionName(section_name, 64, L"AltMoveAccelerometer", move_index);
    config[SensorAxisIndex_X] = profileReadAxisSettings(config_path, section_name, L"X", 0, L"1.0", 300);
    config[SensorAxisIndex_Y] = profileReadAxisSettings(config_path, section_name, L"Y", 0, L"1.0", 300);
    config[SensorAxisIndex_Z] = profileReadAxisSettings(config_path, section_name, L"Z", 0, L"1.0", 500);
}

static AxisSettings profileReadMoveTriggerConfig(const wchar_t *config_path, int move_index)
{
    wchar_t section_name[64];
    makeMoveTriggerSectionName(section_name, 64, move_index);
    return profileReadAxisSettings(config_path, section_name, L"", 0, L"1.0", 0);
}

static AxisSettings profileReadAltMoveTriggerConfig(const wchar_t *config_path, int move_index)
{
    wchar_t section_name[64];
    makeAltMoveTriggerSectionName(section_name, 64, move_index);
    return profileReadAxisSettings(config_path, section_name, L"", 0, L"1.0", 0);
}

static AxisSettings profileReadNavigationTriggerConfig(const wchar_t *config_path, int navigator_index)
{
    wchar_t section_name[64];
    makeNavigationTriggerSectionName(section_name, 64, navigator_index);
    return profileReadAxisSettings(config_path, section_name, L"", 0, L"1.0", 0);
}

static AxisSettings profileReadAltNavigationTriggerConfig(const wchar_t *config_path, int navigator_index)
{
    wchar_t section_name[64];
    makeAltNavigationTriggerSectionName(section_name, 64, navigator_index);
    return profileReadAxisSettings(config_path, section_name, L"", 0, L"1.0", 0);
}

static MoveButtonInput resolveMoveButtonInputFromName(const wchar_t *name)
{
    if (!name || name[0] == L'\0') return MoveButtonInput_Count;
    if (_wcsicmp(name, L"CROSS") == 0) return MoveButtonInput_Cross;
    if (_wcsicmp(name, L"CIRCLE") == 0) return MoveButtonInput_Circle;
    if (_wcsicmp(name, L"SQUARE") == 0) return MoveButtonInput_Square;
    if (_wcsicmp(name, L"TRIANGLE") == 0) return MoveButtonInput_Triangle;
    if (_wcsicmp(name, L"SELECT") == 0 || _wcsicmp(name, L"BACK") == 0) return MoveButtonInput_Select;
    if (_wcsicmp(name, L"START") == 0) return MoveButtonInput_Start;
    if (_wcsicmp(name, L"PS") == 0 || _wcsicmp(name, L"GUIDE") == 0) return MoveButtonInput_PS;
    if (_wcsicmp(name, L"MOVE") == 0) return MoveButtonInput_Move;
    if (_wcsicmp(name, L"TRIGGER") == 0 || _wcsicmp(name, L"T") == 0) return MoveButtonInput_Trigger;
    return MoveButtonInput_Count;
}

static SensorAxisInput resolveSensorAxisInputFromName(const wchar_t *name)
{
    if (!name || name[0] == L'\0') return SensorAxisInput_Count;
#define AXCMP(txt,val) if (_wcsicmp(name, L##txt) == 0) return val
    AXCMP("GYRO_X", SensorAxisInput_GyroX); AXCMP("GYRO_X+", SensorAxisInput_GyroX_Pos); AXCMP("GYRO_X-", SensorAxisInput_GyroX_Neg);
    AXCMP("GYRO_Y", SensorAxisInput_GyroY); AXCMP("GYRO_Y+", SensorAxisInput_GyroY_Pos); AXCMP("GYRO_Y-", SensorAxisInput_GyroY_Neg);
    AXCMP("GYRO_Z", SensorAxisInput_GyroZ); AXCMP("GYRO_Z+", SensorAxisInput_GyroZ_Pos); AXCMP("GYRO_Z-", SensorAxisInput_GyroZ_Neg);
    AXCMP("ACCEL_X", SensorAxisInput_AccelX); AXCMP("ACCEL_X+", SensorAxisInput_AccelX_Pos); AXCMP("ACCEL_X-", SensorAxisInput_AccelX_Neg);
    AXCMP("ACCEL_Y", SensorAxisInput_AccelY); AXCMP("ACCEL_Y+", SensorAxisInput_AccelY_Pos); AXCMP("ACCEL_Y-", SensorAxisInput_AccelY_Neg);
    AXCMP("ACCEL_Z", SensorAxisInput_AccelZ); AXCMP("ACCEL_Z+", SensorAxisInput_AccelZ_Pos); AXCMP("ACCEL_Z-", SensorAxisInput_AccelZ_Neg);
#undef AXCMP
    return SensorAxisInput_Count;
}

static ButtonBinding *getOrCreateMoveButtonBinding(MoveProfile *profile, MoveButtonInput input)
{
    int i;
    if (!profile || input >= MoveButtonInput_Count) return NULL;
    for (i = 0; i < profile->buttonBindingCount; ++i) {
        if (profile->buttonBindings[i].input == input) return &profile->buttonBindings[i];
    }
    if (profile->buttonBindingCount >= MAX_BOUND_INPUTS) return NULL;
    profile->buttonBindings[profile->buttonBindingCount].input = input;
    profile->buttonBindings[profile->buttonBindingCount].actionCount = 0;
    ZeroMemory(profile->buttonBindings[profile->buttonBindingCount].actions, sizeof(profile->buttonBindings[profile->buttonBindingCount].actions));
    ++profile->buttonBindingCount;
    return &profile->buttonBindings[profile->buttonBindingCount - 1];
}

static ButtonBinding *getOrCreateAltMoveButtonBinding(MoveProfile *profile, MoveButtonInput input)
{
    int i;
    if (!profile || input >= MoveButtonInput_Count) return NULL;
    for (i = 0; i < profile->altButtonBindingCount; ++i) {
        if (profile->altButtonBindings[i].input == input) return &profile->altButtonBindings[i];
    }
    if (profile->altButtonBindingCount >= MAX_BOUND_INPUTS) return NULL;
    profile->altButtonBindings[profile->altButtonBindingCount].input = input;
    profile->altButtonBindings[profile->altButtonBindingCount].actionCount = 0;
    ZeroMemory(profile->altButtonBindings[profile->altButtonBindingCount].actions, sizeof(profile->altButtonBindings[profile->altButtonBindingCount].actions));
    ++profile->altButtonBindingCount;
    return &profile->altButtonBindings[profile->altButtonBindingCount - 1];
}

static bool addMoveButtonAction(ButtonBinding *binding, const ButtonAction *action)
{
    if (!binding || !action || binding->actionCount >= MAX_ACTIONS_PER_INPUT) return false;
    binding->actions[binding->actionCount] = *action;
    binding->actions[binding->actionCount].isDown = false;
    binding->actions[binding->actionCount].lastRepeatTick = 0;
    ++binding->actionCount;
    return true;
}

static bool addMoveSensorAction(SensorBinding *binding, const SensorAction *action)
{
    if (!binding || !action || binding->actionCount >= MAX_SENSOR_ACTIONS) return false;
    binding->actions[binding->actionCount] = *action;
    binding->actions[binding->actionCount].isDown = false;
    binding->actions[binding->actionCount].lastRepeatTick = 0;
    ++binding->actionCount;
    return true;
}

static void profileParseMoveSensorBindingValue(SensorBinding bindings[SensorAxisInput_Count], const wchar_t *key_name, const wchar_t *value_text)
{
    wchar_t value_copy[MAX_BIND_TEXT];
    wchar_t *context = NULL;
    wchar_t *token;
    SensorAxisInput axis_input = resolveSensorAxisInputFromName(key_name);

    if (!bindings || !key_name || !value_text || axis_input == SensorAxisInput_Count) return;

    ZeroMemory(value_copy, sizeof(value_copy));
    wcsncpy(value_copy, value_text, MAX_BIND_TEXT - 1);
    trimWideStringInPlace(value_copy);
    if (value_copy[0] == L'\0') return;

    token = wcstok_s(value_copy, L" \t", &context);
    while (token) {
        ActionKind kind = ActionKind_None;
        UINT value = 0;

        trimWideStringInPlace(token);
            if (parseBindingToken(token, &kind, &value)) {
            SensorBinding *binding = &bindings[axis_input];
            SensorAction action;
            ZeroMemory(&action, sizeof(action));
            binding->input = axis_input;
            action.kind = kind;
            if (kind == ActionKind_Keyboard) action.data.vkCode = value;
            else if (kind == ActionKind_MouseButton) action.data.mouseButton = (MouseButtonKind)value;
            else if (kind == ActionKind_MouseWheel) action.data.mouseWheel = (value == 1) ? 1 : -1;
            else if (kind == ActionKind_XboxButton) action.data.xboxButton = (USHORT)value;
            else if (kind == ActionKind_XboxTrigger || kind == ActionKind_XboxAxis || kind == ActionKind_XboxAxis_Pos || kind == ActionKind_XboxAxis_Neg) action.data.xboxAxis = (XboxAxisKind)value;
            addMoveSensorAction(binding, &action);
        }
        token = wcstok_s(NULL, L" \t", &context);
    }
}

static void profileParseMoveBindingValue(MoveProfile *profile, const wchar_t *key_name, const wchar_t *value_text)
{
    MoveButtonInput button_input = resolveMoveButtonInputFromName(key_name);
    SensorAxisInput axis_input = resolveSensorAxisInputFromName(key_name);

    if (!profile || !key_name || !value_text) return;
    if (button_input == MoveButtonInput_Count && axis_input == SensorAxisInput_Count) return;
    if (button_input == MoveButtonInput_Count) {
        profileParseMoveSensorBindingValue(profile->sensorBindings, key_name, value_text);
        return;
    }

    {
        wchar_t value_copy[MAX_BIND_TEXT];
        wchar_t *context = NULL;
        wchar_t *token;

        ZeroMemory(value_copy, sizeof(value_copy));
        wcsncpy(value_copy, value_text, MAX_BIND_TEXT - 1);
        trimWideStringInPlace(value_copy);
        if (value_copy[0] == L'\0') return;

        token = wcstok_s(value_copy, L" \t", &context);
        while (token) {
            ActionKind kind = ActionKind_None;
            UINT value = 0;

            trimWideStringInPlace(token);
            if (parseBindingToken(token, &kind, &value)) {
                ButtonBinding *binding = getOrCreateMoveButtonBinding(profile, button_input);
                ButtonAction action;
                if (binding) {
                    ZeroMemory(&action, sizeof(action));
                    action.kind = kind;
                    if (kind == ActionKind_Keyboard) action.data.vkCode = value;
                    else if (kind == ActionKind_MouseButton) action.data.mouseButton = (MouseButtonKind)value;
                    else if (kind == ActionKind_MouseWheel) action.data.mouseWheel = (value == 1) ? 1 : -1;
                    else if (kind == ActionKind_XboxButton) action.data.xboxButton = (USHORT)value;
                    else if (kind == ActionKind_XboxTrigger || kind == ActionKind_XboxAxis || kind == ActionKind_XboxAxis_Pos || kind == ActionKind_XboxAxis_Neg) action.data.xboxAxis = (XboxAxisKind)value;
                    addMoveButtonAction(binding, &action);
                }
            }
            token = wcstok_s(NULL, L" \t", &context);
        }
    }
}

static void profileParseAltMoveBindingValue(MoveProfile *profile, const wchar_t *key_name, const wchar_t *value_text)
{
    MoveButtonInput button_input = resolveMoveButtonInputFromName(key_name);
    SensorAxisInput axis_input = resolveSensorAxisInputFromName(key_name);

    if (!profile || !key_name || !value_text) return;
    if (button_input == MoveButtonInput_Count && axis_input == SensorAxisInput_Count) return;
    if (button_input == MoveButtonInput_Count) {
        profileParseMoveSensorBindingValue(profile->altSensorBindings, key_name, value_text);
        return;
    }

    {
        wchar_t value_copy[MAX_BIND_TEXT];
        wchar_t *context = NULL;
        wchar_t *token;

        ZeroMemory(value_copy, sizeof(value_copy));
        wcsncpy(value_copy, value_text, MAX_BIND_TEXT - 1);
        trimWideStringInPlace(value_copy);
        if (value_copy[0] == L'\0') return;

        token = wcstok_s(value_copy, L" \t", &context);
        while (token) {
            ActionKind kind = ActionKind_None;
            UINT value = 0;

            trimWideStringInPlace(token);
            if (parseBindingToken(token, &kind, &value)) {
                ButtonBinding *binding = getOrCreateAltMoveButtonBinding(profile, button_input);
                ButtonAction action;
                if (binding) {
                    ZeroMemory(&action, sizeof(action));
                    action.kind = kind;
                    if (kind == ActionKind_Keyboard) action.data.vkCode = value;
                    else if (kind == ActionKind_MouseButton) action.data.mouseButton = (MouseButtonKind)value;
                    else if (kind == ActionKind_MouseWheel) action.data.mouseWheel = (value == 1) ? 1 : -1;
                    else if (kind == ActionKind_XboxButton) action.data.xboxButton = (USHORT)value;
                    else if (kind == ActionKind_XboxTrigger || kind == ActionKind_XboxAxis || kind == ActionKind_XboxAxis_Pos || kind == ActionKind_XboxAxis_Neg) action.data.xboxAxis = (XboxAxisKind)value;
                    addMoveButtonAction(binding, &action);
                }
            }
            token = wcstok_s(NULL, L" \t", &context);
        }
    }
}

static void profileLoadMoveSensorSection(SensorBinding bindings[SensorAxisInput_Count], const wchar_t *section_name, const wchar_t *config_path)
{
    wchar_t section_buffer[INI_SECTION_SIZE];
    wchar_t *entry_ptr;

    if (!bindings || !section_name || !config_path) return;
    ZeroMemory(section_buffer, sizeof(section_buffer));
    GetPrivateProfileSectionW(section_name, section_buffer, INI_SECTION_SIZE, config_path);

    entry_ptr = section_buffer;
    while (*entry_ptr != L'\0') {
        wchar_t *equals_ptr = wcschr(entry_ptr, L'=');
        if (equals_ptr) {
            wchar_t key_name[128];
            wchar_t value_text[MAX_BIND_TEXT];
            size_t key_len = (size_t)(equals_ptr - entry_ptr);
            if (key_len >= 127) key_len = 127;
            wcsncpy(key_name, entry_ptr, key_len);
            key_name[key_len] = L'\0';
            wcsncpy(value_text, equals_ptr + 1, MAX_BIND_TEXT - 1);
            value_text[MAX_BIND_TEXT - 1] = L'\0';
            trimWideStringInPlace(key_name);
            trimWideStringInPlace(value_text);
            profileParseMoveSensorBindingValue(bindings, key_name, value_text);
        }
        entry_ptr += wcslen(entry_ptr) + 1;
    }
}

static void profileLoadAltSection(MoveProfile *profile, const wchar_t *section_name, const wchar_t *config_path)
{
    wchar_t section_buffer[INI_SECTION_SIZE];
    wchar_t *entry_ptr;

    if (!profile || !section_name || !config_path) return;
    ZeroMemory(section_buffer, sizeof(section_buffer));
    GetPrivateProfileSectionW(section_name, section_buffer, INI_SECTION_SIZE, config_path);

    entry_ptr = section_buffer;
    while (*entry_ptr != L'\0') {
        wchar_t *equals_ptr = wcschr(entry_ptr, L'=');
        if (equals_ptr) {
            wchar_t key_name[128];
            wchar_t value_text[MAX_BIND_TEXT];
            size_t key_len = (size_t)(equals_ptr - entry_ptr);
            if (key_len >= 127) key_len = 127;
            wcsncpy(key_name, entry_ptr, key_len);
            key_name[key_len] = L'\0';
            wcsncpy(value_text, equals_ptr + 1, MAX_BIND_TEXT - 1);
            value_text[MAX_BIND_TEXT - 1] = L'\0';
            trimWideStringInPlace(key_name);
            trimWideStringInPlace(value_text);
            profileParseAltMoveBindingValue(profile, key_name, value_text);
        }
        entry_ptr += wcslen(entry_ptr) + 1;
    }
}

static void profileLoadMoveProfile(MoveProfile *profile, int move_index, const wchar_t *config_path)
{
    wchar_t section_name[64];
    wchar_t section_buffer[INI_SECTION_SIZE];
    wchar_t *entry_ptr;

    int i;
    int saved_center[SensorAxisIndex_Count];
    bool saved_captured[SensorAxisIndex_Count];
    if (!profile || move_index < 0 || move_index >= MOVE_COUNT) return;

    for (i = 0; i < SensorAxisIndex_Count; ++i) {
        saved_center[i] = profile->accelCenter[i];
        saved_captured[i] = profile->accelCenterCaptured[i];
    }

    moveProfileResetRuntimeState(profile);
    ZeroMemory(profile, sizeof(*profile));

    for (i = 0; i < SensorAxisIndex_Count; ++i) {
        if (saved_captured[i]) {
            profile->accelCenter[i] = saved_center[i];
            profile->accelCenterCaptured[i] = true;
        }
    }
    profile->repeatMs = profileReadKeyboardKey(config_path);
    profile->mouseRepeatMs = profileReadMouseKey(config_path);
    profile->wheelRepeatMs = profileReadMouseWheelMs(config_path);
    profile->triggerConfig = profileReadMoveTriggerConfig(config_path, move_index);
    profile->altTriggerConfig = profileReadAltMoveTriggerConfig(config_path, move_index);
    profileReadGyroConfig(config_path, move_index, profile->gyroConfig);
    profileReadAccelConfig(config_path, move_index, profile->accelConfig);
    profileReadAltGyroConfig(config_path, move_index, profile->altGyroConfig);
    profileReadAltAccelConfig(config_path, move_index, profile->altAccelConfig);

    makeMoveSectionName(section_name, 64, move_index);
    ZeroMemory(section_buffer, sizeof(section_buffer));
    GetPrivateProfileSectionW(section_name, section_buffer, INI_SECTION_SIZE, config_path);

    entry_ptr = section_buffer;
    while (*entry_ptr != L'\0') {
        wchar_t *equals_ptr = wcschr(entry_ptr, L'=');
        if (equals_ptr) {
            wchar_t key_name[128];
            wchar_t value_text[MAX_BIND_TEXT];
            size_t key_len = (size_t)(equals_ptr - entry_ptr);
            if (key_len >= 127) key_len = 127;
            wcsncpy(key_name, entry_ptr, key_len);
            key_name[key_len] = L'\0';
            wcsncpy(value_text, equals_ptr + 1, MAX_BIND_TEXT - 1);
            value_text[MAX_BIND_TEXT - 1] = L'\0';
            trimWideStringInPlace(key_name);
            trimWideStringInPlace(value_text);
            profileParseMoveBindingValue(profile, key_name, value_text);
        }
        entry_ptr += wcslen(entry_ptr) + 1;
    }

    makeAltMoveSectionName(section_name, 64, move_index);
    profileLoadAltSection(profile, section_name, config_path);
}

static NavigatorButtonInput resolveNavigatorButtonInputFromName(const wchar_t *name)
{
    if (!name || name[0] == L'\0') return NavigatorButtonInput_Count;
#define NCMP(txt,val) if (_wcsicmp(name, L##txt) == 0) return val
    NCMP("CROSS", NavigatorButtonInput_Cross); NCMP("CIRCLE", NavigatorButtonInput_Circle); NCMP("L1", NavigatorButtonInput_L1); NCMP("L2", NavigatorButtonInput_L2); NCMP("L3", NavigatorButtonInput_L3); NCMP("PS", NavigatorButtonInput_PS); NCMP("UP", NavigatorButtonInput_Up); NCMP("RIGHT", NavigatorButtonInput_Right); NCMP("DOWN", NavigatorButtonInput_Down); NCMP("LEFT", NavigatorButtonInput_Left);
#undef NCMP
    return NavigatorButtonInput_Count;
}

static NavigatorAxisInput resolveNavigatorAxisInputFromName(const wchar_t *name)
{
    if (!name || name[0] == L'\0') return NavigatorAxisInput_Count;
    if (_wcsicmp(name, L"X") == 0) return NavigatorAxisInput_X;
    if (_wcsicmp(name, L"X+") == 0) return NavigatorAxisInput_X_Pos;
    if (_wcsicmp(name, L"X-") == 0) return NavigatorAxisInput_X_Neg;
    if (_wcsicmp(name, L"Y") == 0) return NavigatorAxisInput_Y;
    if (_wcsicmp(name, L"Y+") == 0) return NavigatorAxisInput_Y_Pos;
    if (_wcsicmp(name, L"Y-") == 0) return NavigatorAxisInput_Y_Neg;
    return NavigatorAxisInput_Count;
}

static NavigatorButtonBinding *getOrCreateNavigatorButtonBinding(NavigatorProfile *profile, NavigatorButtonInput input)
{
    int i;
    if (!profile || input >= NavigatorButtonInput_Count) return NULL;
    for (i = 0; i < profile->buttonBindingCount; ++i) {
        if (profile->buttonBindings[i].input == input) return &profile->buttonBindings[i];
    }
    if (profile->buttonBindingCount >= MAX_BOUND_INPUTS) return NULL;
    profile->buttonBindings[profile->buttonBindingCount].input = input;
    profile->buttonBindings[profile->buttonBindingCount].actionCount = 0;
    ZeroMemory(profile->buttonBindings[profile->buttonBindingCount].actions, sizeof(profile->buttonBindings[profile->buttonBindingCount].actions));
    ++profile->buttonBindingCount;
    return &profile->buttonBindings[profile->buttonBindingCount - 1];
}

static NavigatorButtonBinding *getOrCreateAltNavigatorButtonBinding(NavigatorProfile *profile, NavigatorButtonInput input)
{
    int i;
    if (!profile || input >= NavigatorButtonInput_Count) return NULL;
    for (i = 0; i < profile->altButtonBindingCount; ++i) {
        if (profile->altButtonBindings[i].input == input) return &profile->altButtonBindings[i];
    }
    if (profile->altButtonBindingCount >= MAX_BOUND_INPUTS) return NULL;
    profile->altButtonBindings[profile->altButtonBindingCount].input = input;
    profile->altButtonBindings[profile->altButtonBindingCount].actionCount = 0;
    ZeroMemory(profile->altButtonBindings[profile->altButtonBindingCount].actions, sizeof(profile->altButtonBindings[profile->altButtonBindingCount].actions));
    ++profile->altButtonBindingCount;
    return &profile->altButtonBindings[profile->altButtonBindingCount - 1];
}

static bool addNavigatorButtonAction(NavigatorButtonBinding *binding, const ButtonAction *action)
{
    if (!binding || !action || binding->actionCount >= MAX_ACTIONS_PER_INPUT) return false;
    binding->actions[binding->actionCount] = *action;
    binding->actions[binding->actionCount].isDown = false;
    binding->actions[binding->actionCount].lastRepeatTick = 0;
    ++binding->actionCount;
    return true;
}

static bool addNavigatorSensorAction(NavigatorSensorBinding *binding, const SensorAction *action)
{
    if (!binding || !action || binding->actionCount >= MAX_SENSOR_ACTIONS) return false;
    binding->actions[binding->actionCount] = *action;
    binding->actions[binding->actionCount].isDown = false;
    binding->actions[binding->actionCount].lastRepeatTick = 0;
    ++binding->actionCount;
    return true;
}

static void profileLoadNavigationStickConfig(NavigatorProfile *profile, int navigator_index, const wchar_t *config_path)
{
    wchar_t section_name[64];

    makeNavigationStickSectionName(section_name, 64, navigator_index);
    profile->stickConfig[0].invert = GetPrivateProfileIntW(section_name, L"InvertX", 0, config_path) ? 1 : 0;
    profile->stickConfig[1].invert = GetPrivateProfileIntW(section_name, L"InvertY", 0, config_path) ? 1 : 0;
    profile->stickConfig[0].sensitivity = profileReadFloat(config_path, section_name, L"SensitivityX", L"1.0");
    profile->stickConfig[1].sensitivity = profileReadFloat(config_path, section_name, L"SensitivityY", L"1.0");
    profile->stickConfig[0].deadzone = GetPrivateProfileIntW(section_name, L"DeadzoneX", 12, config_path);
    profile->stickConfig[1].deadzone = GetPrivateProfileIntW(section_name, L"DeadzoneY", 12, config_path);
}

static void profileLoadAltNavigationStickConfig(NavigatorProfile *profile, int navigator_index, const wchar_t *config_path)
{
    wchar_t section_name[64];

    makeAltNavigationStickSectionName(section_name, 64, navigator_index);
    profile->altStickConfig[0].invert = GetPrivateProfileIntW(section_name, L"InvertX", 0, config_path) ? 1 : 0;
    profile->altStickConfig[1].invert = GetPrivateProfileIntW(section_name, L"InvertY", 0, config_path) ? 1 : 0;
    profile->altStickConfig[0].sensitivity = profileReadFloat(config_path, section_name, L"SensitivityX", L"1.0");
    profile->altStickConfig[1].sensitivity = profileReadFloat(config_path, section_name, L"SensitivityY", L"1.0");
    profile->altStickConfig[0].deadzone = GetPrivateProfileIntW(section_name, L"DeadzoneX", 12, config_path);
    profile->altStickConfig[1].deadzone = GetPrivateProfileIntW(section_name, L"DeadzoneY", 12, config_path);
}

static void profileParseAltNavigatorBindingValue(NavigatorProfile *profile, const wchar_t *key_name, const wchar_t *value_text);

static void profileParseNavigatorBindingValue(NavigatorProfile *profile, const wchar_t *key_name, const wchar_t *value_text)
{
    wchar_t value_copy[MAX_BIND_TEXT];
    wchar_t *context = NULL;
    wchar_t *token;
    NavigatorButtonInput button_input = resolveNavigatorButtonInputFromName(key_name);
    NavigatorAxisInput axis_input = resolveNavigatorAxisInputFromName(key_name);

    if (button_input == NavigatorButtonInput_Count && axis_input == NavigatorAxisInput_Count) return;

    ZeroMemory(value_copy, sizeof(value_copy));
    wcsncpy(value_copy, value_text, MAX_BIND_TEXT - 1);
    trimWideStringInPlace(value_copy);
    if (value_copy[0] == L'\0') return;

    token = wcstok_s(value_copy, L" \t", &context);
    while (token) {
        ActionKind kind = ActionKind_None;
        UINT value = 0;

        trimWideStringInPlace(token);
        if (parseBindingToken(token, &kind, &value)) {
            if (button_input != NavigatorButtonInput_Count) {
                NavigatorButtonBinding *binding = getOrCreateNavigatorButtonBinding(profile, button_input);
                ButtonAction action;
                if (binding) {
                    ZeroMemory(&action, sizeof(action));
                    action.kind = kind;
                    if (kind == ActionKind_Keyboard) action.data.vkCode = value;
                    else if (kind == ActionKind_MouseButton) action.data.mouseButton = (MouseButtonKind)value;
                    else if (kind == ActionKind_MouseWheel) action.data.mouseWheel = (value == 1) ? 1 : -1;
                    else if (kind == ActionKind_XboxButton) action.data.xboxButton = (USHORT)value;
                    else if (kind == ActionKind_XboxTrigger || kind == ActionKind_XboxAxis || kind == ActionKind_XboxAxis_Pos || kind == ActionKind_XboxAxis_Neg) action.data.xboxAxis = (XboxAxisKind)value;
                    addNavigatorButtonAction(binding, &action);
                }
            } else {
                NavigatorSensorBinding *binding = &profile->sensorBindings[axis_input];
                SensorAction action;
                ZeroMemory(&action, sizeof(action));
                binding->input = axis_input;
                action.kind = kind;
                if (kind == ActionKind_Keyboard) action.data.vkCode = value;
                else if (kind == ActionKind_MouseButton) action.data.mouseButton = (MouseButtonKind)value;
                else if (kind == ActionKind_MouseWheel) action.data.mouseWheel = (value == 1) ? 1 : -1;
                else if (kind == ActionKind_XboxButton) action.data.xboxButton = (USHORT)value;
                else if (kind == ActionKind_XboxTrigger || kind == ActionKind_XboxAxis || kind == ActionKind_XboxAxis_Pos || kind == ActionKind_XboxAxis_Neg) action.data.xboxAxis = (XboxAxisKind)value;
                addNavigatorSensorAction(binding, &action);
            }
        }
        token = wcstok_s(NULL, L" \t", &context);
    }
}

static void profileLoadNavigatorProfile(NavigatorProfile *profile, int navigator_index, const wchar_t *config_path)
{
    wchar_t section_name[64];
    wchar_t section_buffer[INI_SECTION_SIZE];
    wchar_t *entry_ptr;

    if (!profile || navigator_index < 0 || navigator_index >= NAVIGATOR_COUNT) return;

    navigatorProfileResetRuntimeState(profile);
    ZeroMemory(profile, sizeof(*profile));
    profile->repeatMs = profileReadKeyboardKey(config_path);
    profile->mouseRepeatMs = profileReadMouseKey(config_path);
    profile->wheelRepeatMs = profileReadMouseWheelMs(config_path);
    profile->triggerConfig = profileReadNavigationTriggerConfig(config_path, navigator_index);
    profile->altTriggerConfig = profileReadAltNavigationTriggerConfig(config_path, navigator_index);
    profileLoadNavigationStickConfig(profile, navigator_index, config_path);
    profileLoadAltNavigationStickConfig(profile, navigator_index, config_path);

    makeNavigationSectionName(section_name, 64, navigator_index);
    ZeroMemory(section_buffer, sizeof(section_buffer));
    GetPrivateProfileSectionW(section_name, section_buffer, INI_SECTION_SIZE, config_path);

    entry_ptr = section_buffer;
    while (*entry_ptr != L'\0') {
        wchar_t *equals_ptr = wcschr(entry_ptr, L'=');
        if (equals_ptr) {
            wchar_t key_name[128];
            wchar_t value_text[MAX_BIND_TEXT];
            size_t key_len = (size_t)(equals_ptr - entry_ptr);
            if (key_len >= 127) key_len = 127;
            wcsncpy(key_name, entry_ptr, key_len);
            key_name[key_len] = L'\0';
            wcsncpy(value_text, equals_ptr + 1, MAX_BIND_TEXT - 1);
            value_text[MAX_BIND_TEXT - 1] = L'\0';
            trimWideStringInPlace(key_name);
            trimWideStringInPlace(value_text);
            profileParseNavigatorBindingValue(profile, key_name, value_text);
        }
        entry_ptr += wcslen(entry_ptr) + 1;
    }

    makeAltNavigationSectionName(section_name, 64, navigator_index);
    ZeroMemory(section_buffer, sizeof(section_buffer));
    GetPrivateProfileSectionW(section_name, section_buffer, INI_SECTION_SIZE, config_path);

    entry_ptr = section_buffer;
    while (*entry_ptr != L'\0') {
        wchar_t *equals_ptr = wcschr(entry_ptr, L'=');
        if (equals_ptr) {
            wchar_t key_name[128];
            wchar_t value_text[MAX_BIND_TEXT];
            size_t key_len = (size_t)(equals_ptr - entry_ptr);
            if (key_len >= 127) key_len = 127;
            wcsncpy(key_name, entry_ptr, key_len);
            key_name[key_len] = L'\0';
            wcsncpy(value_text, equals_ptr + 1, MAX_BIND_TEXT - 1);
            value_text[MAX_BIND_TEXT - 1] = L'\0';
            trimWideStringInPlace(key_name);
            trimWideStringInPlace(value_text);
            profileParseAltNavigatorBindingValue(profile, key_name, value_text);
        }
        entry_ptr += wcslen(entry_ptr) + 1;
    }
}

static void profileLoadRumbleConfig(AppContext *app)
{
    RumbleConfig *cfg;
    if (!app) return;

    cfg = &app->rumble_config;
    cfg->enabled = GetPrivateProfileIntW(L"Rumble", L"Enabled", 1, app->config_path);
    cfg->combine_when_single_move = GetPrivateProfileIntW(L"Rumble", L"CombineWhenSingleMove", 1, app->config_path);
    cfg->master_strength = profileReadFloat(app->config_path, L"Rumble", L"MasterStrength", L"1.0");

    cfg->large.min_active = GetPrivateProfileIntW(L"Rumble", L"LargeMinActive", 10, app->config_path);
    cfg->large.low_threshold = GetPrivateProfileIntW(L"Rumble", L"LargeLowThreshold", 45, app->config_path);
    cfg->large.mid_threshold = GetPrivateProfileIntW(L"Rumble", L"LargeMidThreshold", 120, app->config_path);
    cfg->large.boost_low = profileReadFloat(app->config_path, L"Rumble", L"LargeBoostLow", L"3.2");
    cfg->large.boost_mid = profileReadFloat(app->config_path, L"Rumble", L"LargeBoostMid", L"1.9");
    cfg->large.boost_high = profileReadFloat(app->config_path, L"Rumble", L"LargeBoostHigh", L"1.15");
    cfg->large.min_output = GetPrivateProfileIntW(L"Rumble", L"LargeMinOutput", 90, app->config_path);

    cfg->small.min_active = GetPrivateProfileIntW(L"Rumble", L"SmallMinActive", 5, app->config_path);
    cfg->small.low_threshold = GetPrivateProfileIntW(L"Rumble", L"SmallLowThreshold", 20, app->config_path);
    cfg->small.mid_threshold = GetPrivateProfileIntW(L"Rumble", L"SmallMidThreshold", 90, app->config_path);
    cfg->small.boost_low = profileReadFloat(app->config_path, L"Rumble", L"SmallBoostLow", L"4.0");
    cfg->small.boost_mid = profileReadFloat(app->config_path, L"Rumble", L"SmallBoostMid", L"2.2");
    cfg->small.boost_high = profileReadFloat(app->config_path, L"Rumble", L"SmallBoostHigh", L"1.2");
    cfg->small.min_output = GetPrivateProfileIntW(L"Rumble", L"SmallMinOutput", 70, app->config_path);

    if (cfg->master_strength < 0.0f) cfg->master_strength = 0.0f;
    if (cfg->master_strength > 4.0f) cfg->master_strength = 4.0f;
    sanitizeRumbleMotorConfig(&cfg->large, 90);
    sanitizeRumbleMotorConfig(&cfg->small, 70);
}

static bool pathIsAbsolute(const wchar_t *path)
{
    if (!path || path[0] == L'\0') return false;
    if (path[0] == L'\\' || path[0] == L'/') return true;
    return path[0] != L'\0' && path[1] == L':';
}

static bool profileResolvePath(const wchar_t *base_dir, const wchar_t *candidate_path, const wchar_t *default_name, wchar_t *out_path, size_t out_count)
{
    int written;

    if (!out_path || out_count == 0) return false;
    out_path[0] = L'\0';

    if (candidate_path && candidate_path[0] != L'\0') {
        if (pathIsAbsolute(candidate_path)) {
            wcsncpy(out_path, candidate_path, out_count - 1);
            out_path[out_count - 1] = L'\0';
            return true;
        }

        if (base_dir && base_dir[0] != L'\0') {
            written = _snwprintf(out_path, out_count, L"%ls\\%ls", base_dir, candidate_path);
            if (written >= 0 && written < (int)out_count) {
                out_path[out_count - 1] = L'\0';
                return true;
            }
        }
    }

    if (base_dir && base_dir[0] != L'\0' && default_name && default_name[0] != L'\0') {
        written = _snwprintf(out_path, out_count, L"%ls\\%ls", base_dir, default_name);
        if (written >= 0 && written < (int)out_count) {
            out_path[out_count - 1] = L'\0';
            return true;
        }
    }

    if (default_name && default_name[0] != L'\0') {
        wcsncpy(out_path, default_name, out_count - 1);
        out_path[out_count - 1] = L'\0';
    }
    return false;
}

bool profileBuildPath(wchar_t *out_path, size_t out_count, const wchar_t *file_name)
{
    DWORD module_len;
    wchar_t module_path[MAX_PATH];
    wchar_t *last_slash;

    if (!out_path || out_count == 0) return false;

    module_len = GetModuleFileNameW(NULL, module_path, MAX_PATH);
    if (module_len == 0 || module_len >= MAX_PATH) {
        wcsncpy(out_path, file_name ? file_name : L"profile.ini", out_count - 1);
        out_path[out_count - 1] = L'\0';
        return false;
    }

    last_slash = wcsrchr(module_path, L'\\');
    if (last_slash) {
        *(last_slash + 1) = L'\0';
        return profileResolvePath(module_path, NULL, file_name ? file_name : L"profile.ini", out_path, out_count);
    } else {
        wcsncpy(out_path, file_name ? file_name : L"profile.ini", out_count - 1);
        out_path[out_count - 1] = L'\0';
    }

    return false;
}

static void profileGetSettingsDir(const AppContext *app, wchar_t *out_dir, size_t out_count)
{
    wchar_t *last_slash;

    if (!out_dir || out_count == 0) return;
    out_dir[0] = L'\0';

    if (!app || app->settings_path[0] == L'\0') return;

    wcsncpy(out_dir, app->settings_path, out_count - 1);
    out_dir[out_count - 1] = L'\0';
    last_slash = wcsrchr(out_dir, L'\\');
    if (last_slash) {
        *last_slash = L'\0';
    } else {
        out_dir[0] = L'\0';
    }
}

static void profileParseAltNavigatorBindingValue(NavigatorProfile *profile, const wchar_t *key_name, const wchar_t *value_text)
{
    wchar_t value_copy[MAX_BIND_TEXT];
    wchar_t *context = NULL;
    wchar_t *token;
    NavigatorButtonInput button_input = resolveNavigatorButtonInputFromName(key_name);
    NavigatorAxisInput axis_input = resolveNavigatorAxisInputFromName(key_name);

    if (button_input == NavigatorButtonInput_Count && axis_input == NavigatorAxisInput_Count) return;

    ZeroMemory(value_copy, sizeof(value_copy));
    wcsncpy(value_copy, value_text, MAX_BIND_TEXT - 1);
    trimWideStringInPlace(value_copy);
    if (value_copy[0] == L'\0') return;

    token = wcstok_s(value_copy, L" \t", &context);
    while (token) {
        ActionKind kind = ActionKind_None;
        UINT value = 0;

        trimWideStringInPlace(token);
        if (parseBindingToken(token, &kind, &value)) {
            if (button_input != NavigatorButtonInput_Count) {
                NavigatorButtonBinding *binding = getOrCreateAltNavigatorButtonBinding(profile, button_input);
                ButtonAction action;
                if (binding) {
                    ZeroMemory(&action, sizeof(action));
                    action.kind = kind;
                    if (kind == ActionKind_Keyboard) action.data.vkCode = value;
                    else if (kind == ActionKind_MouseButton) action.data.mouseButton = (MouseButtonKind)value;
                    else if (kind == ActionKind_MouseWheel) action.data.mouseWheel = (value == 1) ? 1 : -1;
                    else if (kind == ActionKind_XboxButton) action.data.xboxButton = (USHORT)value;
                    else if (kind == ActionKind_XboxTrigger || kind == ActionKind_XboxAxis || kind == ActionKind_XboxAxis_Pos || kind == ActionKind_XboxAxis_Neg) action.data.xboxAxis = (XboxAxisKind)value;
                    addNavigatorButtonAction(binding, &action);
                }
            } else {
                NavigatorSensorBinding *binding = &profile->altSensorBindings[axis_input];
                SensorAction action;
                ZeroMemory(&action, sizeof(action));
                binding->input = axis_input;
                action.kind = kind;
                if (kind == ActionKind_Keyboard) action.data.vkCode = value;
                else if (kind == ActionKind_MouseButton) action.data.mouseButton = (MouseButtonKind)value;
                else if (kind == ActionKind_MouseWheel) action.data.mouseWheel = (value == 1) ? 1 : -1;
                else if (kind == ActionKind_XboxButton) action.data.xboxButton = (USHORT)value;
                else if (kind == ActionKind_XboxTrigger || kind == ActionKind_XboxAxis || kind == ActionKind_XboxAxis_Pos || kind == ActionKind_XboxAxis_Neg) action.data.xboxAxis = (XboxAxisKind)value;
                addNavigatorSensorAction(binding, &action);
            }
        }
        token = wcstok_s(NULL, L" \t", &context);
    }
}

static const wchar_t *profileMakePathRelativeToSettingsDir(const AppContext *app, wchar_t *buffer, size_t buffer_count)
{
    wchar_t settings_dir[MAX_PATH];
    size_t settings_dir_len;

    if (!buffer || buffer_count == 0) return L"profile.ini";
    buffer[0] = L'\0';

    if (!app || app->config_path[0] == L'\0') return L"profile.ini";

    profileGetSettingsDir(app, settings_dir, MAX_PATH);
    if (settings_dir[0] == L'\0') {
        return app->config_path;
    }

    settings_dir_len = wcslen(settings_dir);
    if (_wcsnicmp(app->config_path, settings_dir, settings_dir_len) == 0 &&
        (app->config_path[settings_dir_len] == L'\\' || app->config_path[settings_dir_len] == L'/')) {
        wcsncpy(buffer, app->config_path + settings_dir_len + 1, buffer_count - 1);
        buffer[buffer_count - 1] = L'\0';
        return buffer;
    }

    return app->config_path;
}

void profileSaveSettings(const AppContext *app)
{
    wchar_t profile_path_text[MAX_PATH];

    if (!app || app->settings_path[0] == L'\0') return;

    WritePrivateProfileStringW(
        L"General",
        L"Path",
        profileMakePathRelativeToSettingsDir(app, profile_path_text, MAX_PATH),
        app->settings_path
    );
    WritePrivateProfileStringW(
        L"General",
        L"EnableOutput",
        app->output_enabled ? L"1" : L"0",
        app->settings_path
    );
    WritePrivateProfileStringW(
        L"General",
        L"EnableTelemetry",
        app->telemetry_enabled ? L"1" : L"0",
        app->settings_path
    );
    WritePrivateProfileStringW(
        L"General",
        L"UseBthPS3",
        app->use_bthps3 ? L"1" : L"0",
        app->settings_path
    );
    WritePrivateProfileStringW(
        L"General",
        L"UseAutoProfile",
        app->use_auto_profile ? L"1" : L"0",
        app->settings_path
    );
    {
        wchar_t buf[16];
        _snwprintf(buf, 16, L"%d", app->poll_rate_ms);
        buf[15] = L'\0';
        WritePrivateProfileStringW(
            L"General",
            L"PollRateMs",
            buf,
            app->settings_path
        );
    }
}

void profileLoadSettings(AppContext *app)
{
    wchar_t profile_path_text[MAX_PATH];
    wchar_t module_dir[MAX_PATH];
    wchar_t *last_slash;

    if (!app) return;

    profileBuildPath(app->settings_path, MAX_PATH, L"settings.ini");
    profileBuildPath(app->config_path, MAX_PATH, L"profile.ini");
    app->output_enabled = true;

    wcsncpy(module_dir, app->settings_path, MAX_PATH - 1);
    module_dir[MAX_PATH - 1] = L'\0';
    last_slash = wcsrchr(module_dir, L'\\');
    if (last_slash) {
        *last_slash = L'\0';
    } else {
        module_dir[0] = L'\0';
    }

    ZeroMemory(profile_path_text, sizeof(profile_path_text));
    GetPrivateProfileStringW(L"General", L"Path", L"profile.ini", profile_path_text, MAX_PATH, app->settings_path);
    trimWideStringInPlace(profile_path_text);
    profileResolvePath(module_dir, profile_path_text, L"profile.ini", app->config_path, MAX_PATH);

    app->output_enabled = GetPrivateProfileIntW(L"General", L"EnableOutput", 1, app->settings_path) ? true : false;
    app->telemetry_enabled = GetPrivateProfileIntW(L"General", L"EnableTelemetry", 0, app->settings_path) ? true : false;
    app->use_bthps3 = GetPrivateProfileIntW(L"General", L"UseBthPS3", 0, app->settings_path) ? true : false;
    app->use_auto_profile = GetPrivateProfileIntW(L"General", L"UseAutoProfile", 0, app->settings_path) ? true : false;
    app->poll_rate_ms = (int)GetPrivateProfileIntW(L"General", L"PollRateMs", 4, app->settings_path);
    if (app->poll_rate_ms < 1) app->poll_rate_ms = 1;
    if (app->poll_rate_ms > 100) app->poll_rate_ms = 100;
    profileSaveSettings(app);
}

void profileReloadSettings(AppContext *app)
{
    wchar_t profile_path_text[MAX_PATH];
    wchar_t module_dir[MAX_PATH];
    wchar_t *last_slash;
    wchar_t new_config_path[MAX_PATH];
    bool path_changed;

    if (!app) return;

    app->output_enabled = GetPrivateProfileIntW(L"General", L"EnableOutput", 1, app->settings_path) ? true : false;
    app->telemetry_enabled = GetPrivateProfileIntW(L"General", L"EnableTelemetry", 0, app->settings_path) ? true : false;
    app->use_auto_profile = GetPrivateProfileIntW(L"General", L"UseAutoProfile", 0, app->settings_path) ? true : false;
    app->poll_rate_ms = (int)GetPrivateProfileIntW(L"General", L"PollRateMs", 4, app->settings_path);
    if (app->poll_rate_ms < 1) app->poll_rate_ms = 1;
    if (app->poll_rate_ms > 100) app->poll_rate_ms = 100;

    wcsncpy(module_dir, app->settings_path, MAX_PATH - 1);
    module_dir[MAX_PATH - 1] = L'\0';
    last_slash = wcsrchr(module_dir, L'\\');
    if (last_slash) {
        *last_slash = L'\0';
    } else {
        module_dir[0] = L'\0';
    }

    ZeroMemory(profile_path_text, sizeof(profile_path_text));
    GetPrivateProfileStringW(L"General", L"Path", L"profile.ini", profile_path_text, MAX_PATH, app->settings_path);
    trimWideStringInPlace(profile_path_text);
    profileResolvePath(module_dir, profile_path_text, L"profile.ini", new_config_path, MAX_PATH);

    path_changed = _wcsicmp(app->config_path, new_config_path) != 0;
    if (path_changed) {
        wcsncpy(app->config_path, new_config_path, MAX_PATH - 1);
        app->config_path[MAX_PATH - 1] = L'\0';
        profileSaveSettings(app);
    }

    logWrite("settings", "reloaded: Emulation=%d Telemetry=%d PollRateMs=%d PathChanged=%d",
        app->output_enabled, app->telemetry_enabled, app->poll_rate_ms, path_changed);
}

BOOL profileIsTelemetryEnabled(const AppContext *app)
{
    if (!app) return FALSE;
    return app->telemetry_enabled;
}

void profileLoadAll(AppContext *app)
{
    int move_index;
    int navigator_index;

    if (!app) return;

    for (move_index = 0; move_index < MOVE_COUNT; ++move_index) {
        profileLoadMoveProfile(&app->move_profiles[move_index], move_index, app->config_path);
    }

    for (navigator_index = 0; navigator_index < NAVIGATOR_COUNT; ++navigator_index) {
        profileLoadNavigatorProfile(&app->navigator_profiles[navigator_index], navigator_index, app->config_path);
    }
    profileLoadRumbleConfig(app);
    logWriteW("config", L"profile.ini reloaded: %ls", app->config_path);
}
