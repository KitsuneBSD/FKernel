#pragma once

#include <LibC/stdio.h>
#include <LibFK/log.h>

namespace FK {

[[noreturn]] inline void panic(char const* message) noexcept
{
    Logf(LogLevel::ERROR, "%s", message);
    while (true)
        asm volatile("cli; hlt");
}

inline void alert(char const* message) noexcept
{
    Logf(LogLevel::WARN, "%s", message);
}

inline void enforce(bool condition, char const* message) noexcept
{
    if (condition) [[likely]] {
        return;
    }

    char buffer[512];
    LibC::snprintf(buffer, sizeof(buffer),
        "PANIC at %s:%d in %s(): %s",
        __FILE__, __LINE__, __func__, message);

    panic(buffer);
}

inline void enforce(bool condition, char const* message,
    char const* file, int line, char const* func) noexcept
{
    if (condition) [[likely]] {
        return;
    }

    char buffer[512];
    LibC::snprintf(buffer, sizeof(buffer),
        "PANIC at %s:%d in %s(): %s",
        file, line, func, message);

    panic(buffer);
}

inline void enforcef(bool condition, char const* format, ...) noexcept
{
    if (condition) [[likely]] {
        return;
    }

    char msg[384];
    char buffer[512];

    va_list args;
    va_start(args, format);
    LibC::vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);

    LibC::snprintf(buffer, sizeof(buffer),
        "PANIC at %s:%d in %s(): %s",
        __FILE__, __LINE__, __func__, msg);

    panic(buffer);
}

inline void enforcef(bool condition, char const* format,
    char const* file, int line, char const* func, ...) noexcept
{
    if (condition) [[likely]] {
        return;
    }

    char msg[384];
    char buffer[512];

    va_list args;
    va_start(args, func);
    LibC::vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);

    LibC::snprintf(buffer, sizeof(buffer),
        "PANIC at %s:%d in %s(): %s",
        file, line, func, msg);

    panic(buffer);
}

inline bool alert_if(bool condition, char const* message) noexcept
{
    if (condition) {
        alert(message);
    }

    return condition;
}
inline bool alert_if_f(bool condition, char const* message, ...) noexcept
{
    if (condition) [[likely]] {
        char buffer[512];
        LibC::snprintf(buffer, sizeof(buffer),
            "ALERT at %s:%d in %s(): %s",
            __FILE__, __LINE__, __func__, message);

        alert(buffer);
    }

    return condition;
}

inline bool alert_if(bool condition, char const* message,
    char const* file, int line, char const* func) noexcept
{
    if (condition) [[likely]] {
        char buffer[512];
        LibC::snprintf(buffer, sizeof(buffer),
            "ALERT at %s:%d in %s(): %s",
            file, line, func, message);

        alert(buffer);
    }

    return condition;
}

inline bool alert_if_f(bool condition, char const* format,
    char const* file, int line, char const* func, ...) noexcept
{
    if (condition) [[likely]] {
        char msg[384];
        char buffer[512];

        va_list args;
        va_start(args, func);
        LibC::vsnprintf(msg, sizeof(msg), format, args);
        va_end(args);

        LibC::snprintf(buffer, sizeof(buffer),
            "ALERT at %s:%d in %s(): %s",
            file, line, func, msg);

        alert(buffer);
    }

    return condition;
}

} // namespace FK
