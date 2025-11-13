#include <LibC/stdarg.h>
#include <LibC/string.h>
#include <LibFK/Types/types.h>

extern "C" int vsnprintf(char *buf, size_t size, const char *fmt,
                         va_list args) {
  size_t pos = 0;

  auto write_char = [&](char c) {
    if (pos + 1 < size)
      buf[pos++] = c;
  };

  for (size_t i = 0; fmt[i] && pos + 1 < size; i++) {
    if (fmt[i] != '%') {
      write_char(fmt[i]);
      continue;
    }

    i++;
    char pad_char = ' ';
    int width = 0;

    if (fmt[i] == '0') {
      pad_char = '0';
      i++;
    }

    while (fmt[i] >= '0' && fmt[i] <= '9') {
      width = width * 10 + (fmt[i] - '0');
      i++;
    }

    bool is_long = false;
    bool is_size_t = false;

    if (fmt[i] == 'l') {
      is_long = true;
      i++;
    } else if (fmt[i] == 'z') {
      is_size_t = true;
      i++;
    }

    char spec = fmt[i];
    char temp[65];
    temp[0] = '\0';

    switch (spec) {
    case 'c': {
      char c = (char)va_arg(args, int);
      temp[0] = c;
      temp[1] = '\0';
      break;
    }
    case 's': {
      const char *s = va_arg(args, const char *);
      strncpy(temp, s, sizeof(temp) - 1);
      temp[sizeof(temp) - 1] = '\0';
      break;
    }
    case 'd': {
      long val = is_long ? va_arg(args, long) : va_arg(args, int);
      itoa_signed(val, temp, 10);
      break;
    }
    case 'u': {
      unsigned long val;
      if (is_long)
        val = va_arg(args, unsigned long);
      else if (is_size_t)
        val = va_arg(args, size_t);
      else
        val = va_arg(args, unsigned int);
      itoa_unsigned(val, temp, 10, false);
      break;
    }
    case 'x': {
      unsigned long val;
      if (is_long)
        val = va_arg(args, unsigned long);
      else if (is_size_t)
        val = va_arg(args, size_t);
      else
        val = va_arg(args, unsigned int);
      itoa_unsigned(val, temp, 16, false);
      break;
    }
    case 'p': {
      uintptr_t val = (uintptr_t)va_arg(args, void *);
      itoa_unsigned(val, temp, 16, false);
      break;
    }
    case '%': {
      temp[0] = '%';
      temp[1] = '\0';
      break;
    }
    default: {
      temp[0] = '%';
      temp[1] = spec;
      temp[2] = '\0';
      break;
    }
    }

    size_t len = strlen(temp);
    int padding = (width > (int)len) ? (width - (int)len) : 0;
    for (int p = 0; p < padding && pos + 1 < size; p++)
      write_char(pad_char);

    for (size_t p = 0; p < len && pos + 1 < size; p++)
      write_char(temp[p]);
  }

  buf[pos] = '\0';
  return (int)pos;
}
