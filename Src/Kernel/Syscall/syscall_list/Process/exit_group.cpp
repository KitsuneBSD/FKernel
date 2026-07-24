#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>

extern "C" {
uint64_t sys_exit(uint64_t status, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs);

uint64_t sys_exit_group(uint64_t status, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, [[maybe_unused]] PtRegs* regs) {
    return sys_exit(status, a1, a2, a3, a4, a5, regs);
}
}
