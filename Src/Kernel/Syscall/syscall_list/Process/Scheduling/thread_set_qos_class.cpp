#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <Kernel/Scheduler/Sync/turnstile.h>
#include <Kernel/Syscall/syscall.h>

using namespace fkernel::scheduler;

// sys_thread_set_qos_class(...) → 0 or -errno
extern "C" uint64_t sys_thread_set_qos_class(uint64_t pid, uint64_t qos_class,
                                              uint64_t, uint64_t, uint64_t, uint64_t,
                                              [[maybe_unused]] PtRegs* regs) {
  auto target = SchedulerManager::the().find_task(fk::ProcessId(pid));
  if (!target.get()) return (uint64_t)-3;
  if (qos_class > 5) return (uint64_t)-22;

  QoSClass qos = static_cast<QoSClass>(qos_class);
  target->control.lifecycle.qos = qos;
  target->control.lifecycle.base_priority =
      priority_for_qos(qos, fk::NiceValue(target->control.lifecycle.nice)).value();
  target->control.lifecycle.priority = target->control.lifecycle.base_priority;
  return 0;
}
