#include <LibC/stdio.h>
#include <LibC/stdarg.h>

int snprintf(char *str, size_t size, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int result = vsnprintf(str, size, fmt, args);
  va_end(args);
  return result;
}