#include <LibC/stdarg.h>
#include <LibC/stdio.h>
#include <LibC/string.h>

static inline void itoa_unsigned(uint64_t value, char *buf, int base,
                                 bool uppercase) {
  char tmp[65];
  const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
  int i = 0;

  if (value == 0) {
    tmp[i++] = '0';
  } else {
    while (value) {
      tmp[i++] = digits[value % base];
      value /= base;
    }
  }

  int j = 0;
  while (i > 0) {
    buf[j++] = tmp[--i];
  }
  buf[j] = '\0';
}

static inline void itoa_signed(int64_t value, char *buf, int base) {
  if (value < 0) {
    *buf++ = '-';
    itoa_unsigned((uint64_t)(-value), buf, base, false);
  } else {
    itoa_unsigned((uint64_t)value, buf, base, false);
  }
}

extern "C" int snprintf(char *str, size_t size, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t i = 0;
  size_t pos = 0;

  while (fmt[i] && pos < size - 1) {
    if (fmt[i] == '%') {
      i++;
      if (fmt[i] == 'd') {
        int val = va_arg(args, int);
        char num_buf[32];
        int len = itoa(val, num_buf, 10);
        for (int j = 0; j < len && pos < size - 1; j++) {
          str[pos++] = num_buf[j];
        }
      } else if (fmt[i] == 'x') {
        int val = va_arg(args, int);
        char num_buf[32];
        int len = itoa(val, num_buf, 16);
        for (int j = 0; j < len && pos < size - 1; j++) {
          str[pos++] = num_buf[j];
        }
      } else if (fmt[i] == 's') {
        char *s = va_arg(args, char *);
        while (*s && pos < size - 1) {
          str[pos++] = *s++;
        }
      } else {
        str[pos++] = '%';
        if (pos < size - 1)
          str[pos++] = fmt[i];
      }
    } else {
      str[pos++] = fmt[i];
    }
    i++;
  }

  str[pos] = '\0';
  va_end(args);
  return pos;
}

extern "C" void kprintf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  for (size_t i = 0; fmt[i]; ++i) {
    if (fmt[i] != '%') {
      serial::write_char(fmt[i]);
      vga::the().put_char(fmt[i]);
      continue;
    }

    if (!fmt[i + 1])
      break;

    ++i;
    char spec = fmt[i];
    switch (spec) {
    case 's': {
      const char *s = va_arg(args, const char *);
      serial::write(s);
      vga::the().write(s);
      break;
    }
    case 'd': {
      int val = va_arg(args, int);
      serial::write_dec(val);
      char buf[12];
      snprintf(buf, sizeof(buf), "%d", val);
      vga::the().write(buf);
      break;
    }
    case 'x': {
      unsigned int val = va_arg(args, unsigned int);
      serial::write_hex(val);
      char buf[12];
      snprintf(buf, sizeof(buf), "%x", val);
      vga::the().write(buf);
      break;
    }
    case 'u': {
      unsigned int val = va_arg(args, unsigned int);
      char buf[12];
      itoa(val, buf, 10);
      serial::write(buf);
      vga::the().write(buf);
      break;
    }
    case 'l': {
      char next = fmt[i + 1];
      if (next == 'u') {
        unsigned long val = va_arg(args, unsigned long);
        char buf[24];
        ultoa(val, buf, 10);
        serial::write(buf);
        vga::the().write(buf);
        i++; // avançar o 'u'
      } else if (next == 'x') {
        unsigned long val = va_arg(args, unsigned long);
        char buf[24];
        ultoa(val, buf, 16);
        serial::write(buf);
        vga::the().write(buf);
        i++;
      } else {
        serial::write_char('%');
        vga::the().put_char('%');
        serial::write_char('l');
        vga::the().put_char('l');
      }
      break;
    }
    case 'p': {
      void *ptr = va_arg(args, void *);
      uintptr_t val = reinterpret_cast<uintptr_t>(ptr);
      char buf[24];
      ultoa(val, buf, 16);
      serial::write("0x");
      vga::the().write("0x");
      serial::write(buf);
      vga::the().write(buf);
      break;
    }

    case '%': {
      serial::write_char('%');
      vga::the().put_char('%');
      break;
    }
    default: {
      serial::write_char('%');
      serial::write_char(spec);
      vga::the().put_char('%');
      vga::the().put_char(spec);
      break;
    }
    }
  }

  va_end(args);
}

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

    if (fmt[i] == 'l') {
      is_long = true;
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
    case '%': {
      buf[pos++] = '%';
      break;
    }
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
