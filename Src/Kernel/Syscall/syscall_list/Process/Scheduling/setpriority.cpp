#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel::scheduler;

// sys_setpriority(...) → 0 or -errno
extern "C" uint64_t sys_setpriority([[maybe_unused]] uint64_t which,
                                     [[maybe_unused]] uint64_t who,
                                     uint64_t prio, uint64_t, uint64_t, uint64_t,
                                     [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;
  int nice = 20 - (int)(int64_t)prio;
  if (nice < -20) nice = -20;
  if (nice > 19) nice = 19;
  task->control.lifecycle.nice = (int8_t)nice;
  task->control.lifecycle.base_priority =
      priority_for_qos(task->control.lifecycle.qos, fk::NiceValue(task->control.lifecycle.nice)).value();
  task->control.lifecycle.priority = task->control.lifecycle.base_priority;
  return 0;
}
