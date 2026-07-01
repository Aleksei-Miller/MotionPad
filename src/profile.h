#ifndef PROFILE_H
#define PROFILE_H

#include "app_context.h"

float profileReadFloat(const wchar_t *config_path, const wchar_t *section, const wchar_t *key, const wchar_t *default_value);
AxisSettings profileReadAxisSettings(const wchar_t *config_path, const wchar_t *section_name, const wchar_t *axis_suffix, int default_invert, const wchar_t *default_sensitivity, int default_deadzone);
int profileReadKeyboardKey(const wchar_t *config_path);
int profileReadMouseKey(const wchar_t *config_path);
int profileReadMouseWheelMs(const wchar_t *config_path);

bool profileBuildPath(wchar_t *out_path, size_t out_count, const wchar_t *file_name);
void profileLoadSettings(AppContext *app);
void profileReloadSettings(AppContext *app);
void profileSaveSettings(const AppContext *app);
void profileLoadAll(AppContext *app);
BOOL profileIsTelemetryEnabled(const AppContext *app);

void trimWideStringInPlace(wchar_t *str);

#endif
