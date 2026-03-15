#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

extern "C" {
uint64_t sys_setpgid([[maybe_unused]] uint64_t pid, [[maybe_unused]] uint64_t pgid, uint64_t,
                     uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  return 0;
}
}
