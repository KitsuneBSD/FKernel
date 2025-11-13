#include <LibC/stdarg.h>
#include <LibC/stdio.h>
#include <LibC/string.h>

extern "C" void kprintf(const char *fmt, ...) {
  char buffer[512];
  va_list args;
  va_start(args, fmt);

  vsnprintf(buffer, sizeof(buffer), fmt, args);

  va_end(args);

  serial::write(buffer);
  vga::the().write_ansi(buffer);
}
