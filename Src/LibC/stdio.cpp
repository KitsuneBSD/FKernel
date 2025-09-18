#include <LibC/stdio.h>
#include <LibC/string.h>

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
      // Você pode usar sua própria função aqui:
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
