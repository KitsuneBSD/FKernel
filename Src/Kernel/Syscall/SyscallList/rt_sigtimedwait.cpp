#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>

extern "C" uint64_t sys_rt_sigtimedwait(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    // Stub for rt_sigtimedwait. Return success (0).
    return 0;
}
