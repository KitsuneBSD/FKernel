#include "Driver/Vga_Buffer.hpp"
#include <LibFK/Log.h>

using namespace vga;

LogLevel Logger::currentLevel = LogLevel::INFO;

Logger &Logger::Instance() {
  static Logger instance;
  return instance;
}

const char *Logger::LevelToString(LogLevel level) {
  switch (level) {
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
