#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel::scheduler;

// sys_getpriority(...) → 0 or -errno
extern "C" uint64_t sys_getpriority([[maybe_unused]] uint64_t which,
                                     [[maybe_unused]] uint64_t who,
                                     uint64_t, uint64_t, uint64_t, uint64_t,
                                     [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;
  return (uint64_t)(uint32_t)(20 - task->control.lifecycle.nice);
}
