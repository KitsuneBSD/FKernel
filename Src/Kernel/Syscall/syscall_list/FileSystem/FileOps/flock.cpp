#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

// Advisory file locking — all lock requests succeed immediately (no enforcement).
// Programs use flock for inter-process coordination; since FKernel is single-task,
// the semantic guarantee holds trivially.
extern "C" uint64_t sys_flock(uint64_t fd, uint64_t operation,
                               uint64_t, uint64_t, uint64_t, uint64_t,
                               [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;
  auto desc = task->get_file_descriptor((int)fd);
  if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
  (void)operation;
  return 0;
}
