#include <LibC/stdio.h>

#include <LibC/stdarg.h>
#include <LibC/stdbool.h>
#include <LibC/stdint.h>

void kprintf(const char *fmt, ...) {
  char buf[512];
  if (!fmt) return;
  va_list args;
  va_start(args, fmt);
  int chars_written = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if ((size_t)chars_written >= sizeof(buf)) {
    libc_puts("kprintf: Output truncated!\n");
  }

  libc_puts(buf);
}
