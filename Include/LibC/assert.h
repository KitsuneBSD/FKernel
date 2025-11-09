#pragma once

#include <LibC/stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void __kernel_assert_fail(const char *expr, const char *file, int line,
                          const char *func);

#define ASSERT(expr)                                                           \
  ((expr) ? (void)0 : __kernel_assert_fail(#expr, __FILE__, __LINE__, __func__))

#ifdef __cplusplus
}
#endif
