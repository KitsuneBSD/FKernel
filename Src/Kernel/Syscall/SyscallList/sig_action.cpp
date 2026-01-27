#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>

extern "C" {

uint64_t sys_sigaction(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                       uint64_t, PtRegs* regs) {

  return 0;
}
}
