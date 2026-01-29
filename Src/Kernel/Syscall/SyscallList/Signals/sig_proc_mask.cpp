#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>

extern "C" {

uint64_t sys_sigprocmask(uint64_t how, uint64_t set_ptr, uint64_t oldset_ptr,
                         uint64_t /* sigsetsize */, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto *task = SchedulerManager::the().current();
  if (!task)
    return -1;

  if (oldset_ptr) {
    // Assume sigsetsize is 8 for x86_64
    *reinterpret_cast<uint64_t *>(oldset_ptr) = task->ipc.signals.mask;
  }

  if (set_ptr) {
    uint64_t new_set = *reinterpret_cast<uint64_t *>(set_ptr);
    if (how == 0) { // SIG_BLOCK
      task->ipc.signals.mask |= new_set;
    } else if (how == 1) { // SIG_UNBLOCK
      task->ipc.signals.mask &= ~new_set;
    } else if (how == 2) { // SIG_SETMASK
      task->ipc.signals.mask = new_set;
    }
  }

  return 0;
}
}
