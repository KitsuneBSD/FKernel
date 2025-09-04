#pragma once

#include "Kernel/Driver/Vga/Vga_buffer.h"
#include <Kernel/Driver/SerialPort/Serial.h>

#include <LibC/stdarg.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void kprintf(const char *fmt, ...) {

  va_list args;
  va_start(args, fmt);

  for (size_t i = 0; fmt[i]; ++i) {
    if (fmt[i] != '%') {
      Serial::write_char(fmt[i]);
      VGA::instance().put_char(fmt[i]);
      continue;
    }

    ++i; // pula o '%'
    char spec = fmt[i];
    switch (spec) {
    case 's': {
      const char *s = va_arg(args, const char *);
      Serial::write(s);
      VGA::instance().write(s);
      break;
    }
    case 'd': {
      int val = va_arg(args, int);
      Serial::write_dec(val);
      break;
    }
    case 'x': {
      unsigned int val = va_arg(args, unsigned int);
      Serial::write_hex(val);
      break;
    }
    case '%': {
      Serial::write_char('%');
      break;
    }
    default: {
      Serial::write_char('%');
      Serial::write_char(spec);
      break;
    }
    }
  }

  va_end(args);
}

#ifdef __cplusplus
}
#endif
