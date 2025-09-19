#pragma once

#include <LibC/stdarg.h>
#include <LibC/stdio.h>

#define KLOG_COLOR_RESET "\033[0m"
#define KLOG_COLOR_RED "\033[31m"
#define KLOG_COLOR_GREEN "\033[32m"
#define KLOG_COLOR_YELLOW "\033[33m"
#define KLOG_COLOR_BLUE "\033[34m"
#define KLOG_COLOR_MAGENTA "\033[35m"
#define KLOG_COLOR_CYAN "\033[36m"
#define KLOG_COLOR_WHITE "\033[37m"

#ifdef FKERNEL_DEBUG
inline void klog(const char *prefix, const char *fmt, ...) {
  char buf[512]; // buffer temporário para a string formatada
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args); // formata a string
  va_end(args);

  // imprime com prefixo colorido (verde por padrão)
  kprintf("%s[%s]%s: %s\n", KLOG_COLOR_GREEN, prefix, KLOG_COLOR_RESET, buf);
}

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
#define klog(...)       // nada em release
#define klog_color(...) // nada em release
#endif
