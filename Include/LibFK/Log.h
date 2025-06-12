#pragma once

#include <Driver/Vga_Buffer.hpp>

enum class LogLevel : uint8_t {
  INFO = 1,
  WARN = 2,
  ERROR = 3,
};

class Logger {
private:
  static LogLevel currentLevel;
  static const char *LevelToString(LogLevel level);
  static vga::Color LevelToColor(LogLevel level);

public:
  static Logger &Instance();
  static void SetLevel(LogLevel level);

  void Log(LogLevel level, const char *message) const;
};

inline void Log(LogLevel level, const char *message) {
  Logger::Instance().Log(level, message);
}
