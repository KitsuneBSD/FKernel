#include <LibC/stdarg.h>
#include <LibC/stdio.h>
#include <LibC/string.h>

extern "C" int vsnprintf(char *buf, size_t size, const char *fmt,
                         va_list args) {
  size_t pos = 0;

  for (size_t i = 0; fmt[i] && pos + 1 < size; i++) {
    if (fmt[i] != '%') {
      buf[pos++] = fmt[i];
      continue;
    }

    i++;
    bool is_long = false;
    bool is_size_t = false;

    if (fmt[i] == 'l') {
      is_long = true;
      i++;
    } else if (fmt[i] == 'z') {
      is_size_t = true;
      i++;
    }

    char temp[65];
    switch (fmt[i]) {
    case 'c': {
      char c = (char)va_arg(args, int);
      if (pos < size - 1)
        buf[pos++] = c;
      break;
    }
    case 's': {
      const char *s = va_arg(args, const char *);
      while (*s && pos + 1 < size)
        buf[pos++] = *s++;
      break;
    }
    case 'd': {
      if (is_long) {
        long val = va_arg(args, long);
        itoa_signed(val, temp, 10);
      } else {
        int val = va_arg(args, int);
        itoa_signed(val, temp, 10);
      }
      for (char *p = temp; *p && pos + 1 < size; p++)
        buf[pos++] = *p;
      break;
    }
    case 'u': {
      if (is_long) {
        unsigned long val = va_arg(args, unsigned long);
        itoa_unsigned(val, temp, 10, false);
      } else if (is_size_t) {
        size_t val = va_arg(args, size_t);
        itoa_size(val, temp, 10);
      } else {
        unsigned int val = va_arg(args, unsigned int);
        itoa_unsigned(val, temp, 10, false);
      }
      for (char *p = temp; *p && pos + 1 < size; p++)
        buf[pos++] = *p;
      break;
    }
    case 'x': {
      if (is_long) {
        unsigned long val = va_arg(args, unsigned long);
        itoa_unsigned(val, temp, 16, false);
      } else if (is_size_t) {
        size_t val = va_arg(args, size_t);
        itoa_size(val, temp, 16);
      } else {
        unsigned int val = va_arg(args, unsigned int);
        itoa_unsigned(val, temp, 16, false);
      }
      for (char *p = temp; *p && pos + 1 < size; p++)
        buf[pos++] = *p;
      break;
    }
    case 'p': {
      uintptr_t val = (uintptr_t)va_arg(args, void *);
      itoa_unsigned(val, temp, 16, false);
      if (pos + 2 < size)
        buf[pos++] = '0';
      if (pos + 2 < size)
        buf[pos++] = 'x';
      for (char *p = temp; *p && pos + 1 < size; p++)
        buf[pos++] = *p;
      break;
    }
    case '%':
      buf[pos++] = '%';
      break;
    default:
      buf[pos++] = '%';
      if (pos + 1 < size)
        buf[pos++] = fmt[i];
      break;
    }
  }

  buf[pos] = '\0';
  return (int)pos;
}
