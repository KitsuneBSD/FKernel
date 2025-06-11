#pragma once

#include <Driver/Vga_Buffer.hpp>

enum class LogLevel : uint8_t {
  WARN = 1,
  INFO = 0,
  ERROR = 2,
};

class Logger {
private:
  static LogLevel currentLevel;

  static const char *LevelToString(LogLevel level);
  static vga::Color LevelToColor(LogLevel level);

public:
  static void SetLevel(LogLevel level);
  static void Log(LogLevel level, const char *message);
};
