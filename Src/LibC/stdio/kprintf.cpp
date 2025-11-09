#include <LibC/stdarg.h>
#include <LibC/stdio.h>
#include <LibC/string.h>

// TODO: Add support to '/r'
extern "C" void kprintf(const char *fmt, ...) {
  char buffer[512];
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
  vga::the().write_ansi(buffer);
}
