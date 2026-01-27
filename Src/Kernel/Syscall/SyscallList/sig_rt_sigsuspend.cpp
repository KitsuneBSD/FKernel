#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/log.h>

extern "C" {

uint64_t sys_rt_sigsuspend(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                           uint64_t, PtRegs* regs) {

  // For now, block until a signal (or wakeup).
  // In our simple model, sys_exit of child will wake us up.
  auto* task = SchedulerManager::the().current();
  fk::algorithms::klog("SYSCALL", "sigsuspend: PID %lu blocking", task ? task->id : 0);
  SchedulerManager::the().block_current();
  SchedulerManager::the().schedule();

  return fkernel::return_error(fk::core::Error::Interrupted);
}
}
