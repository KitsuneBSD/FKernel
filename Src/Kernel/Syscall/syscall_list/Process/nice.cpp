#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/qos.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel::scheduler;

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

extern "C" uint64_t sys_getpriority([[maybe_unused]] uint64_t which,
                                     [[maybe_unused]] uint64_t who,
                                     uint64_t, uint64_t, uint64_t, uint64_t,
                                     [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;
  return (uint64_t)(uint32_t)(20 - task->control.lifecycle.nice);
}

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
