#include <LibC/stdio.h>

#include <LibC/assert.h>
#include <LibC/stdarg.h>
#include <LibC/stdbool.h>
#include <LibC/stdint.h>

void kprintf(const char *fmt, ...) {
  char buf[512];
  ASSERT(fmt != NULL);
  ASSERT(sizeof(buf) >= 32); // Ensure a reasonable buffer size for kprintf
  va_list args;
  va_start(args, fmt);
  int chars_written = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if ((size_t)chars_written >= sizeof(buf)) {
    // Log a warning if truncation occurred
    libc_puts("kprintf: Output truncated!\n");
  }

  libc_puts(buf);
}
