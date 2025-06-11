#include "Driver/Vga_Buffer.hpp"
#include <LibFK/Log.h>

using namespace vga;

LogLevel Logger::currentLevel = LogLevel::INFO;

void Logger::SetLevel(LogLevel level) { currentLevel = level; }

void Logger::Log(LogLevel level, const char *message) {
  if (static_cast<uint8_t>(level) < static_cast<uint8_t>(currentLevel)) {
    return; // Filter out logs below the current level
  }

  vga::Color color = LevelToColor(level);
  const char *levelStr = LevelToString(level);

  console.set_color(color, vga::Color::Black);
  console.write("[");
  console.write(levelStr);
  console.write("] ");
  console.write(message);
  console.write("\n");
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
    return vga::Color::LightGreen;
  case LogLevel::WARN:
    return vga::Color::Yellow;
  case LogLevel::ERROR:
    return vga::Color::Red;
  default:
    return vga::Color::White;
  }
}
