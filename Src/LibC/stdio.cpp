#include "LibC/string.h"
#include <LibC/stdio.h>

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char *buf = out;
  while (*fmt) {
    if (*fmt == '%') {
      ++fmt;
      switch (*fmt) {
      case 'c': {
        char c = (char)va_arg(args, int);
        *buf++ = c;
        break;
      }
      case 's': {
        const char *s = va_arg(args, const char *);
        while (*s)
          *buf++ = *s++;
        break;
      }
      case 'd':
      case 'i': {
        int val = va_arg(args, int);
        if (val < 0) {
          *buf++ = '-';
          val = -val;
        }
        buf += utoa((unsigned int)val, buf, 10);
        break;
      }
      case 'u': {
        unsigned int val = va_arg(args, unsigned int);
        buf += utoa(val, buf, 10);
        break;
      }
      case 'x': {
        unsigned int val = va_arg(args, unsigned int);
        buf += utoa(val, buf, 16);
        break;
      }
      default:
        *buf++ = '%';
        *buf++ = *fmt;
      }
    } else {
      *buf++ = *fmt;
    }
    ++fmt;
  }

  *buf = '\0';
  va_end(args);
  return buf - out;
}
