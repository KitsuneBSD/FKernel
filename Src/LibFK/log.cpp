#include <Kernel/Driver/SerialPort.h>
#include <LibFK/log.h>

#include <LibC/stdarg.h>
#include <LibC/stdio.h>

LogLevel Logger::currentLevel = LogLevel::INFO;

Logger& Logger::Instance()
{
    static Logger instance;
    return instance;
}

char const* Logger::LevelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::TRACE:
        return "TRACE";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARN:
        return "WARN";
    case LogLevel::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

void Logger::SetLevel(LogLevel level) { currentLevel = level; }

void Logger::Log(LogLevel level, char const* message) const
{
    if (level < currentLevel)
        return;

    auto& serial = Serial::SerialPort::Instance();

    serial.write("[");
    serial.write(LevelToString(level));
    serial.write("] ");
    serial.write(message);
    serial.write("\n");
}
void Logger::Logf(LogLevel level, char const* fmt, ...) const
{
    if (level < currentLevel)
        return;

    char buffer[512];
    va_list args;
    va_start(args, fmt);
    LibC::vsprintf(buffer, fmt, args);
    va_end(args);

    Log(level, buffer);
}
