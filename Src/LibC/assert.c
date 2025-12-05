#include <LibC/assert.h>
#include <LibC/stdio.h>

void __kernel_assert_fail(const char *expr, const char *file, int line,
                          const char *func) {
  kprintf("Assert Failed: %s (%s: %s: %d)\n", expr, file, func, line);

  while (1) {
    __asm__ volatile("cli; hlt");
  }
}
