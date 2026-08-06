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
  __builtin_trap();
}

/**
 * __cxa_finalize stub
 */
void __cxa_finalize(void *f) { (void)f; }

extern "C" int __cxa_guard_acquire(uint64_t *guard) {
  auto* guard_byte = reinterpret_cast<uint8_t*>(guard);
  // CAS 0→1: wins the init race, returns 1 (caller should initialize)
  if (__sync_bool_compare_and_swap(guard_byte, (uint8_t)0, (uint8_t)1))
    return 1;
  // Lost the race — spin until the winner marks it done (0xFF); acquire load for ARM/RISC-V portability
  while (__atomic_load_n(guard_byte, __ATOMIC_ACQUIRE) == 1)
    asm volatile("pause" ::: "memory"); /* L7: x86 pause hint; replace with arch_cpu_relax() in Phase 42 */
  return 0;
}

extern "C" void __cxa_guard_release(uint64_t *guard) {
  __sync_synchronize();
  *reinterpret_cast<uint8_t*>(guard) = 0xFF;
}

extern "C" void __cxa_guard_abort(uint64_t *guard) {
  *reinterpret_cast<uint8_t*>(guard) = 0;
  __sync_synchronize();
}
}
