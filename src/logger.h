#ifndef LOGGER_H
#define LOGGER_H

#include "app_common.h"

bool loggerInit(void);
void loggerClose(void);
void loggerWriteA(const char *category, const char *fmt, ...);
void loggerWriteW(const char *category, const wchar_t *fmt, ...);

#define logWrite  loggerWriteA
#define logWriteW loggerWriteW

#endif
