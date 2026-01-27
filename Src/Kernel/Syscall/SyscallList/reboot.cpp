#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_reboot(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs* regs) {
    // Stub for reboot syscall. Return success to allow init to proceed.
    return 0;
}
