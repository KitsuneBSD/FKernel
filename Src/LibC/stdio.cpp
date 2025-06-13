#include "LibC/string.h"
#include <LibC/stdio.h>

static char *format_integer(char *buf, int64_t val, int base) {
  if (val < 0) {
    *buf++ = '-';
    val = -val;
  }
  return buf + utoa((uint64_t)val, buf, base);
}

static char *format_unsigned(char *buf, uint64_t val, int base) {
  return buf + utoa(val, buf, base);
}

static char *format_string(char *buf, const char *str) {
  while (*str)
    *buf++ = *str++;
  return buf;
}

static char *format_char(char *buf, char c) {
  *buf++ = c;
  return buf;
}

static char *format_pointer(char *buf, void *ptr) {
  const char *prefix = "0x";
  while (*prefix)
    *buf++ = *prefix++;
  return format_unsigned(buf, (uintptr_t)ptr, 16);
}

int vsprintf(char *out, const char *fmt, va_list args) {
  char *buf = out;

  while (*fmt) {
    if (*fmt == '%') {
      ++fmt;

      bool long_flag = false;
      bool long_long_flag = false;

      if (*fmt == 'l') {
        ++fmt;
        if (*fmt == 'l') {
          ++fmt;
          long_long_flag = true;
        } else {
          long_flag = true;
        }
      }

      switch (*fmt) {
      case 'c':
        buf = format_char(buf, (char)va_arg(args, int));
        break;

      case 's':
        buf = format_string(buf, va_arg(args, const char *));
        break;

      case 'd':
      case 'i': {
        int64_t val = long_long_flag ? va_arg(args, long long)
                      : long_flag    ? va_arg(args, long)
                                     : va_arg(args, int);
        buf = format_integer(buf, val, 10);
        break;
      }

      case 'u': {
        uint64_t val = long_long_flag ? va_arg(args, unsigned long long)
                       : long_flag    ? va_arg(args, unsigned long)
                                      : va_arg(args, unsigned int);
        buf = format_unsigned(buf, val, 10);
        break;
      }

      case 'x':
      case 'X': {
        uint64_t val = long_long_flag ? va_arg(args, unsigned long long)
                       : long_flag    ? va_arg(args, unsigned long)
                                      : va_arg(args, unsigned int);
        buf = format_unsigned(buf, val, 16);
        break;
      }

      case 'p':
        buf = format_pointer(buf, va_arg(args, void *));
        break;

      default:
        *buf++ = '%';
        *buf++ = *fmt;
        break;
      }
    } else {
      *buf++ = *fmt;
    }
    ++fmt;
  }

  *buf = '\0';
  return buf - out;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int written = vsprintf(out, fmt, args);
  va_end(args);
  return written;
}
