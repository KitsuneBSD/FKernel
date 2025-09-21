#include <LibC/assert.h>
#include <LibFK/log.h>

extern "C" void __kernel_assert_fail(const char *expr, const char *file,
                                     int line, const char *func) {
  klog("ASSERT", KLOG_COLOR_RED, "Failed: %s (%s: %s: %d)\n", expr, file, func,
       line);

#ifdef FKERNEL_DEBUG
  while (1) {
    __asm__ volatile("cli; hlt");
  }
#endif

  __builtin_unreachable();
}
