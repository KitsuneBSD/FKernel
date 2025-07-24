#include <Kernel/Driver/SerialPort/SerialPort.h>
#include <LibFK/log.h>

#include <LibC/stdarg.h>
#include <LibC/stdio.h>

LogLevel Logger::currentLevel = LogLevel::INFO;

static bool enableColors = true;

static char const* LevelToColor(LogLevel level)
{
    switch (level) {
    case LogLevel::TRACE:
        return "\x1b[34m"; // Azul
    case LogLevel::INFO:
        return "\x1b[32m"; // Verde
    case LogLevel::WARN:
        return "\x1b[33m"; // Amarelo
    case LogLevel::ERROR:
        return "\x1b[31m"; // Vermelho
    default:
        return "\x1b[0m"; // Reset
    }
}

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
        return "TRACE";
    }
}

void Logger::SetLevel(LogLevel level) { currentLevel = level; }

void Logger::Log(LogLevel level, char const* message) const
{
    if (level < currentLevel)
        return;

    auto& serial = Serial::SerialPort::Instance();

    char const* levelStr = LevelToString(level);

    if (enableColors) {
        char const* color = LevelToColor(level);
        serial.write(color);
        serial.write("[");
        serial.write(levelStr);
        serial.write("]");
        serial.write("\x1b[0m "); // Reset color
    } else {
        serial.write("[");
        serial.write(levelStr);
        serial.write("] ");
    }

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
