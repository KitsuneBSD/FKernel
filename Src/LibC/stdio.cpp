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
  char buffer[512]; // tamanho arbitrário para formatação
  size_t buf_index = 0;

  auto write_char_to_buffer = [&](char c) {
    if (buf_index < sizeof(buffer) - 1) {
      buffer[buf_index++] = c;
    }
  };

  va_list args;
  va_start(args, fmt);

  for (size_t i = 0; fmt[i]; ++i) {
    if (fmt[i] != '%') {
      write_char_to_buffer(fmt[i]);
      continue;
    }

    if (!fmt[i + 1])
      break;

    ++i;
    char spec = fmt[i];
    switch (spec) {
    case 's': {
      const char *s = va_arg(args, const char *);
      while (*s)
        write_char_to_buffer(*s++);
      break;
    }
    case 'd': {
      int val = va_arg(args, int);
      char numbuf[12];
      snprintf(numbuf, sizeof(numbuf), "%d", val);
      for (char *p = numbuf; *p; ++p)
        write_char_to_buffer(*p);
      break;
    }
    case 'x': {
      unsigned int val = va_arg(args, unsigned int);
      char numbuf[12];
      snprintf(numbuf, sizeof(numbuf), "%x", val);
      for (char *p = numbuf; *p; ++p)
        write_char_to_buffer(*p);
      break;
    }
    case 'u': {
      unsigned int val = va_arg(args, unsigned int);
      char numbuf[12];
      snprintf(numbuf, sizeof(numbuf), "%u", val);
      for (char *p = numbuf; *p; ++p)
        write_char_to_buffer(*p);
      break;
    }
    case 'p': {
      void *ptr = va_arg(args, void *);
      uintptr_t val = reinterpret_cast<uintptr_t>(ptr);
      char numbuf[20];
      itoa_unsigned(val, numbuf, 16, false); // converte para hexadecimal
      write_char_to_buffer('0');
      write_char_to_buffer('x');
      for (char *p = numbuf; *p; ++p)
        write_char_to_buffer(*p);
      break;
    }

    case '%':
      write_char_to_buffer('%');
      break;
    default:
      write_char_to_buffer('%');
      write_char_to_buffer(spec);
      break;
    }
  }

  va_end(args);

  buffer[buf_index] = '\0';

  serial::write(buffer);
  vga::the().write(buffer);
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
