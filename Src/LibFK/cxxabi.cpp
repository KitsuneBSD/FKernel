#include <LibC/stddef.h>
#include <LibC/stdint.h>

typedef void (*atexit_fn_t)();

extern "C" {

/**
 * @brief Mock implementation of atexit for freestanding kernel.
 *
 * Does nothing and always returns 0.
 */
int atexit(atexit_fn_t func) {
  (void)func; // ignorar o ponteiro
  return 0;
}

/**
 * __cxa_atexit alias (C++ global destructors)
 * Some compilers emit calls to __cxa_atexit instead of atexit
 */
int __cxa_atexit(atexit_fn_t func, void *arg, void *dso_handle) {
  (void)func;
  (void)arg;
  (void)dso_handle;
  return 0;
}

/**
 * __cxa_finalize stub
 */
void __cxa_finalize(void *f) { (void)f; }
}
