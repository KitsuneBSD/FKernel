#include "Driver/Vga_Buffer.hpp"
#include <LibFK/Log.h>

#include <LibC/stdarg.h>
#include <LibC/stdio.h>

using namespace vga;

LogLevel Logger::currentLevel = LogLevel::INFO;

Logger &Logger::Instance() {
  static Logger instance;
  return instance;
}

const char *Logger::LevelToString(LogLevel level) {
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

vga::Color Logger::LevelToColor(LogLevel level) {
  switch (level) {
  case LogLevel::TRACE:
    return Color::LightGray;
  case LogLevel::INFO:
    return Color::LightGreen;
  case LogLevel::WARN:
    return Color::Yellow;
  case LogLevel::ERROR:
    return Color::LightRed;
  default:
    return Color::White;
  }
}

void Logger::SetLevel(LogLevel level) { currentLevel = level; }

void Logger::Log(LogLevel level, const char *message) const {
  if (level < currentLevel)
    return;

  console.set_color(LevelToColor(level), Color::Black);
  console.write(LevelToString(level));
  console.write(": ");
  console.write(message);

  console.write("\n");
}
void Logger::Logf(LogLevel level, const char *fmt, ...) const {
  if (level < currentLevel)
    return;

  char buffer[512];
  va_list args;
  va_start(args, fmt);
  vsprintf(buffer, fmt, args);
  va_end(args);

  Log(level, buffer);
}
