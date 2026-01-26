#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_rt_sigtimedwait(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    // Stub for rt_sigtimedwait. Return success (0).
    return 0;
}
