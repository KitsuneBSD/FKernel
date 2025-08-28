#pragma once

#include <Kernel/Driver/SerialPort/SerialPort.h>
#include <LibC/stdarg.h>
#include <LibC/stdio.h>

enum class LogLevel : LibC::uint8_t {
    TRACE = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
};

class Logger {
private:
    static LogLevel currentLevel;
    static char const* LevelToString(LogLevel level);

public:
    static Logger& Instance();
    static void SetLevel(LogLevel level);

    void Log(LogLevel level, char const* message) const;

    void Logf(LogLevel level, char const* fmt, ...) const;
};

inline void Logf(LogLevel level, char const* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buffer[512];
    LibC::vsprintf(buffer, fmt, args);
    va_end(args);
    Logger::Instance().Log(level, buffer);
}

inline void Log(LogLevel level, char const* message)
{
    Logger::Instance().Log(level, message);
}
