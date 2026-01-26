#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_reboot(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    // Stub for reboot syscall. Return success to allow init to proceed.
    return 0;
}
