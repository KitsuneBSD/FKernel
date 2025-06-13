#pragma once

#include <LibC/stdarg.h>
#include <LibC/string.h>

int sprintf(char *out, const char *fmt, ...);
int vsprintf(char *out, const char *fmt, va_list args);
