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

      char pad_char = ' ';
      if (*fmt == '0') {
        pad_char = '0';
        ++fmt;
      }

      int width = 0;
      while (*fmt >= '0' && *fmt <= '9') {
        width = width * 10 + (*fmt - '0');
        ++fmt;
      }

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

      char temp[65];
      size_t len = 0;

      switch (*fmt) {
      case 'c':
        *buf++ = (char)va_arg(args, int);
        break;

      case 's': {
        const char *str = va_arg(args, const char *);
        while (*str)
          *buf++ = *str++;
        break;
      }

      case 'd':
      case 'i': {
        int64_t val = long_long_flag ? va_arg(args, long long)
                      : long_flag    ? va_arg(args, long)
                                     : va_arg(args, int);
        if (val < 0) {
          *buf++ = '-';
          val = -val;
        }
        len = utoa(val, temp, 10);
        break;
      }

      case 'u': {
        uint64_t val = long_long_flag ? va_arg(args, unsigned long long)
                       : long_flag    ? va_arg(args, unsigned long)
                                      : va_arg(args, unsigned int);
        len = utoa(val, temp, 10);
        break;
      }

      case 'x':
      case 'X': {
        uint64_t val = long_long_flag ? va_arg(args, unsigned long long)
                       : long_flag    ? va_arg(args, unsigned long)
                                      : va_arg(args, unsigned int);
        len = utoa(val, temp, 16);
        break;
      }

      case 'p': {
        uintptr_t val = (uintptr_t)va_arg(args, void *);
        *buf++ = '0';
        *buf++ = 'x';
        len = utoa(val, temp, 16);
        break;
      }

      default:
        *buf++ = '%';
        *buf++ = *fmt;
        break;
      }

      // Apply padding if necessary
      if (len > 0) {
        for (int i = (int)len; i < width; ++i)
          *buf++ = pad_char;
        for (size_t i = 0; i < len; ++i)
          *buf++ = temp[i];
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
