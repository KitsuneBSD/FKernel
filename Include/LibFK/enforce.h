#pragma once

#include "LibC/stdio.h"
#include <LibFK/log.h>

namespace FK {

[[noreturn]] inline void panic(char const* message) noexcept
{
    Logf(LogLevel::ERROR, "PANIC: %s", message);
    while (true)
        __builtin_trap(); // Ou `hlt` se quiser inline assembly x86
}

inline void alert(char const* message) noexcept
{
    Logf(LogLevel::WARN, "ALERT: %s", message);
}

inline void enforce(bool condition, char const* message) noexcept
{
    if (!condition) [[unlikely]] {
        panic(message);
    }
}
inline void enforcef(bool condition, char const* format, ...) noexcept
{
    if (condition) [[unlikely]] {
        return;
    }

    char buffer[512];

    va_list args;
    va_start(args, format);
    LibC::vsprintf(buffer, format, args); // assume vsprintf escreve buffer corretamente
    va_end(args);

    panic(buffer);
}

inline void alert_if(bool condition, char const* message) noexcept
{
    if (condition) {
        alert(message);
    }
}
inline void alert_if_f(bool condition, char const* format, ...) noexcept
{
    if (condition) {
        char buffer[512];

        va_list args;
        va_start(args, format);
        LibC::vsprintf(buffer, format, args); // assume vsprintf escreve buffer corretamente
        va_end(args);

        alert(buffer);
        return;
    }
}
}
