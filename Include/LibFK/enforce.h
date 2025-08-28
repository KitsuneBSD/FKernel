#pragma once

#include <LibC/stdio.h>
#include <LibFK/log.h>

namespace FK {

inline void panic(char const* message) noexcept
{
    Logf(LogLevel::ERROR, "%s", message);
    while (true)
        asm volatile("cli; hlt");
}

inline void alert(char const* message) noexcept
{
    Logf(LogLevel::TRACE, "%s", message);
}

inline void enforce(bool condition, char const* message) noexcept
{
    if (condition) [[likely]]
        return;

    char buffer[512];
    LibC::snprintf(buffer, sizeof(buffer),
        "PANIC: %s", message);
    panic(buffer);
}

inline void enforcef(bool condition, char const* format, ...) noexcept
{
    if (condition) [[likely]]
        return;

    char msg[384];
    char buffer[512];

    va_list args;
    va_start(args, format);
    LibC::vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);

    LibC::snprintf(buffer, sizeof(buffer),
        "PANIC on: %s", msg);
    panic(buffer);
}

inline bool alert_if(bool condition, char const* message) noexcept
{
    if (condition) {
        char buffer[512];
        LibC::snprintf(buffer, sizeof(buffer),
            "ALERT: %s", message);
        alert(buffer);
    }
    return condition;
}

inline bool alert_if_f(bool condition, char const* format, ...) noexcept
{
    char msg[384];
    char buffer[512];

    if (condition) {
        va_list args;
        va_start(args, format);
        LibC::vsnprintf(msg, sizeof(msg), format, args);
        va_end(args);

        msg[sizeof(msg) - 1] = '\0'; // Segurança contra truncamento

        LibC::snprintf(buffer, sizeof(buffer),
            "ALERT: %s", msg);
        alert(buffer);
    }

    return condition;
}

} // namespace FK
