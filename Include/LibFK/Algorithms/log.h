#pragma once

#include <LibC/stdarg.h>
#include <LibC/stdio.h>

namespace fk {
namespace algorithms { // Or fk::logging if preferred

/** ANSI color codes for kernel logging */
#define KLOG_COLOR_RESET "\033[0m"
#define KLOG_COLOR_RED "\033[31m"
#define KLOG_COLOR_GREEN "\033[32m"
#define KLOG_COLOR_YELLOW "\033[33m"
#define KLOG_COLOR_BLUE "\033[34m"
#define KLOG_COLOR_MAGENTA "\033[35m"
#define KLOG_COLOR_CYAN "\033[36m"
#define KLOG_COLOR_WHITE "\033[37m"

/**
 * @brief Print a formatted kernel log message in red and block by default.
 *
 *
 * @param prefix Prefix to display before the message (e.g., module name)
 * @param fmt printf-style format string
 * @param ... Variadic arguments for formatting
 */
inline void kerror(const char *prefix, const char *fmt, ...) {
  char buf[512]; ///< Temporary buffer for formatted message
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  kprintf("%s[%s]%s: %s\n", KLOG_COLOR_RED, prefix, KLOG_COLOR_RESET, buf);

  while (true) {
    asm("cli;hlt");
  }
}
/**
 * @brief Print a formatted kernel log message in red by default.
 *
 *
 * @param prefix Prefix to display before the message (e.g., module name)
 * @param fmt printf-style format string
 * @param ... Variadic arguments for formatting
 */
inline void kexception(const char *prefix, const char *fmt, ...) {
  char buf[512]; ///< Temporary buffer for formatted message
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  kprintf("%s[%s]%s: %s\n", KLOG_COLOR_RED, prefix, KLOG_COLOR_RESET, buf);
}

inline void kwarn(const char *prefix, const char *fmt, ...) {
  char buf[512]; ///< Temporary buffer for formatted message
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  kprintf("%s[%s]%s: %s\n", KLOG_COLOR_YELLOW, prefix, KLOG_COLOR_RESET, buf);
}

inline void kdebug(const char *prefix, const char *fmt, ...) {
  char buf[512]; ///< Temporary buffer for formatted message
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  kprintf("%s[%s]%s: %s\n", KLOG_COLOR_WHITE, prefix, KLOG_COLOR_RESET, buf);
}

#ifdef FKERNEL_DEBUG

/**
 * @brief Print a formatted kernel log message in green by default.
 *
 * This function only works in debug mode. In release mode, it is omitted.
 *
 * @param prefix Prefix to display before the message (e.g., module name)
 * @param fmt printf-style format string
 * @param ... Variadic arguments for formatting
 */
inline void klog(const char *prefix, const char *fmt, ...) {
  char buf[512]; ///< Temporary buffer for formatted message
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  kprintf("%s[%s]%s: %s\n", KLOG_COLOR_GREEN, prefix, KLOG_COLOR_RESET, buf);
}

/**
 * @brief Print a formatted kernel log message in a specified ANSI color.
 *
 * This function only works in debug mode. In release mode, it is omitted.
 *
 * @param prefix Prefix to display before the message
 * @param color ANSI color code to use for the prefix
 * @param fmt printf-style format string
 * @param ... Variadic arguments for formatting
 */
inline void klog_color(const char *prefix, const char *color, const char *fmt,
                       ...) {
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  kprintf("%s[%s]%s: %s\n", color, prefix, KLOG_COLOR_RESET, buf);
}
#else
/// In release mode, kebug does nothing
#define kdebug(...)
/// In release mode, klog does nothing
#define klog(...)
/// In release mode, klog_color does nothing
#define klog_color(...)
#endif

} // namespace algorithms
} // namespace fk