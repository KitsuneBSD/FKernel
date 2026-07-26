#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/qos.h>
#include <Kernel/Scheduler/turnstile.h>
#include <Kernel/Syscall/syscall.h>

using namespace fkernel::scheduler;

extern "C" uint64_t sys_thread_set_qos_class(uint64_t pid, uint64_t qos_class,
                                              uint64_t, uint64_t, uint64_t, uint64_t,
                                              [[maybe_unused]] PtRegs* regs) {
  auto target = SchedulerManager::the().find_task(fk::ProcessId(pid));
  if (!target.get()) return (uint64_t)-3;
  if (qos_class > 5) return (uint64_t)-22;

  QoSClass qos = static_cast<QoSClass>(qos_class);
  target->control.lifecycle.qos = qos;
  target->control.lifecycle.base_priority =
      priority_for_qos(qos, target->control.lifecycle.nice);
  target->control.lifecycle.priority = target->control.lifecycle.base_priority;
  return 0;
}

extern "C" uint64_t sys_thread_get_qos_class(uint64_t pid, uint64_t, uint64_t,
                                              uint64_t, uint64_t, uint64_t,
                                              [[maybe_unused]] PtRegs* regs) {
  auto target = SchedulerManager::the().find_task(fk::ProcessId(pid));
  if (!target.get()) return (uint64_t)-3;
  return static_cast<uint64_t>(target->control.lifecycle.qos);
}
