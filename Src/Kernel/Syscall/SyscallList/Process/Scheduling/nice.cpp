#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel::scheduler;

// sys_nice(...) → 0 or -errno
extern "C" uint64_t sys_nice(uint64_t increment, uint64_t, uint64_t, uint64_t,
                              uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;

  int new_nice = (int)task->control.lifecycle.nice + (int)(int64_t)increment;
  if (new_nice < -20) new_nice = -20;
  if (new_nice > 19) new_nice = 19;
  task->control.lifecycle.nice = (int8_t)new_nice;
  task->control.lifecycle.base_priority =
      priority_for_qos(task->control.lifecycle.qos, fk::NiceValue(task->control.lifecycle.nice)).value();
  task->control.lifecycle.priority = task->control.lifecycle.base_priority;
  return (uint64_t)(uint32_t)new_nice;
}
