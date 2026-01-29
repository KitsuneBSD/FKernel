#include <LibC/stddef.h>
#include <LibC/stdint.h>

typedef void (*atexit_fn_t)();

/**
 * @brief Mock implementation of atexit for freestanding kernel.
 *
 * Does nothing and always returns 0.
 */
extern "C" {
  void *__dso_handle = nullptr;

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

extern "C" void __cxa_pure_virtual() {
  // This should ideally not be called in a freestanding environment.
  // If it is, it indicates a programming error.
  while (true) {
    asm("cli; hlt");
  }
}

/**
 * __cxa_finalize stub
 */
void __cxa_finalize(void *f) { (void)f; }

extern "C" int __cxa_guard_acquire(uint64_t *guard) {
  return !(*guard & 0xFF);
}

extern "C" void __cxa_guard_release(uint64_t *guard) {
  *guard |= 0xFF;
}

extern "C" void __cxa_guard_abort(uint64_t *guard) {
  (void)guard;
}
}
