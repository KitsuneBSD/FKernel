#pragma once

#include <LibC/stdarg.h>
#include <LibC/stdio.h>

namespace fk {
namespace algorithms {

enum LogTarget : uint32_t {
    None = 0,
    Display = 1 << 0,
    Serial = 1 << 1,
    DebugFS = 1 << 2,
    All = 0xFFFFFFFF
};

enum LogLevel : uint32_t {
    LevelTrace = 0,
    LevelDebug = 1,
    LevelInfo  = 2,
    LevelWarn  = 3,
    LevelError = 4,
    LevelFatal = 5,
    LevelSilent = 6
};

void set_log_targets(uint32_t targets);
uint32_t get_log_targets();
void set_log_level(LogLevel level);
LogLevel get_log_level();

#define KLOG_COLOR_RESET "\033[0m"
#define KLOG_COLOR_RED "\033[31m"
#define KLOG_COLOR_GREEN "\033[32m"
#define KLOG_COLOR_YELLOW "\033[33m"
#define KLOG_COLOR_BLUE "\033[34m"
#define KLOG_COLOR_MAGENTA "\033[35m"
#define KLOG_COLOR_CYAN "\033[36m"
#define KLOG_COLOR_WHITE "\033[37m"

/**
 * @brief Print a formatted kernel log message in red and halt the CPU.
 * Use only for truly unrecoverable errors (heap corruption, hardware init failure).
 */
inline void kfatal(const char *prefix, const char *fmt, ...) {
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  kprintf("%s[FATAL][%s]%s: %s\n", KLOG_COLOR_RED, prefix, KLOG_COLOR_RESET, buf);

  while (true) {
    asm("cli;hlt");
  }
}

/**
 * @brief Print a formatted kernel log message in red.
 * Does NOT halt — use kfatal() for unrecoverable errors that must stop execution.
 */
inline void kerror(const char *prefix, const char *fmt, ...) {
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  kprintf("%s[%s]%s: %s\n", KLOG_COLOR_RED, prefix, KLOG_COLOR_RESET, buf);
}

/** Print a formatted kernel log message in red (exception context). */
inline void kexception(const char *prefix, const char *fmt, ...) {
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  kprintf("%s[%s]%s: %s\n", KLOG_COLOR_RED, prefix, KLOG_COLOR_RESET, buf);
}

/**
 * Compile-time log level gate.
 * -DFKERNEL_LOG_LEVEL=0  → only kfatal/kerror (silent release)
 * -DFKERNEL_LOG_LEVEL=1  → +kwarn
 * -DFKERNEL_LOG_LEVEL=2  → +klog  (recommended release)
 * -DFKERNEL_LOG_LEVEL=3  → +kdebug (default debug build)
 * -DFKERNEL_LOG_LEVEL=4  → +ktrace (verbose tracing)
 * Unset → all levels enabled (same as 3).
 */
#ifndef FKERNEL_LOG_LEVEL
# define FKERNEL_LOG_LEVEL 3
#endif

/** Print a formatted kernel log message in yellow. No-op when FKERNEL_LOG_LEVEL < 1. */
#if FKERNEL_LOG_LEVEL >= 1
inline void kwarn(const char *prefix, const char *fmt, ...) {
  if (get_log_level() > LevelWarn) return;
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  kprintf("%s[%s]%s: %s\n", KLOG_COLOR_YELLOW, prefix, KLOG_COLOR_RESET, buf);
}
#else
inline void kwarn(const char*, const char*, ...) {}
#endif

/** Print a formatted kernel log message in white. No-op when FKERNEL_LOG_LEVEL < 3. */
#if FKERNEL_LOG_LEVEL >= 3
inline void kdebug(const char *prefix, const char *fmt, ...) {
  if (get_log_level() > LevelDebug) return;
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  kprintf("%s[%s]%s: %s\n", KLOG_COLOR_WHITE, prefix, KLOG_COLOR_RESET, buf);
}
#else
inline void kdebug(const char*, const char*, ...) {}
#endif

/** Print a formatted kernel log message in green. No-op when FKERNEL_LOG_LEVEL < 2. */
#if FKERNEL_LOG_LEVEL >= 2
inline void klog(const char *prefix, const char *fmt, ...) {
  if (get_log_level() > LevelInfo) return;
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  kprintf("%s[%s]%s: %s\n", KLOG_COLOR_GREEN, prefix, KLOG_COLOR_RESET, buf);
}
#else
inline void klog(const char*, const char*, ...) {}
#endif

/** Print a formatted kernel log message in cyan. No-op when FKERNEL_LOG_LEVEL < 4. */
#if FKERNEL_LOG_LEVEL >= 4
inline void ktrace(const char *prefix, const char *fmt, ...) {
  if (get_log_level() > LevelTrace) return;
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  kprintf("%s[%s]%s: %s\n", KLOG_COLOR_CYAN, prefix, KLOG_COLOR_RESET, buf);
}
#else
inline void ktrace(const char*, const char*, ...) {}
#endif

/** Print a formatted kernel log message in a specified ANSI color. */
inline void klog_color(const char *prefix, const char *color, const char *fmt,
                       ...) {
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  kprintf("%s[%s]%s: %s\n", color, prefix, KLOG_COLOR_RESET, buf);
}

} // namespace algorithms
} // namespace fk
