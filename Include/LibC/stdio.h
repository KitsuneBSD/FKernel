#pragma once

#include <LibC/stdarg.h>
#include <LibC/string.h>

namespace LibC {

int sprintf(char* out, char const* fmt, ...);
int vsprintf(char* out, char const* fmt, va_list args);

}
