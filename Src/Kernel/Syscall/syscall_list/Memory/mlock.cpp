#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>

// Advisory memory locking — always succeeds (no real memory pinning needed
// in FKernel's single-address-space userland context).
extern "C" {
uint64_t sys_mlock(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                   [[maybe_unused]] PtRegs* regs) { return 0; }
uint64_t sys_munlock(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                     [[maybe_unused]] PtRegs* regs) { return 0; }
uint64_t sys_mlockall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                      [[maybe_unused]] PtRegs* regs) { return 0; }
uint64_t sys_munlockall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                        [[maybe_unused]] PtRegs* regs) { return 0; }
}
