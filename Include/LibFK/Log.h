#pragma once

#include <Driver/Vga_Buffer.hpp>
#include <LibC/stdarg.h>
#include <LibC/stdio.h>

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

  void Logf(LogLevel level, const char *fmt, ...) const;
};

inline void Logf(LogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buffer[512];
  vsprintf(buffer, fmt, args);
  va_end(args);
  Logger::Instance().Log(level, buffer);
}

inline void Log(LogLevel level, const char *message) {
  Logger::Instance().Log(level, message);
}
