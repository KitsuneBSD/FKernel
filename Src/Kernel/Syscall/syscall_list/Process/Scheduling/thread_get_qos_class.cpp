#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <Kernel/Scheduler/Sync/turnstile.h>
#include <Kernel/Syscall/syscall.h>

using namespace fkernel::scheduler;

// sys_thread_get_qos_class(...) → 0 or -errno
extern "C" uint64_t sys_thread_get_qos_class(uint64_t pid, uint64_t, uint64_t,
                                              uint64_t, uint64_t, uint64_t,
                                              [[maybe_unused]] PtRegs* regs) {
  auto target = SchedulerManager::the().find_task(fk::ProcessId(pid));
  if (!target.get()) return (uint64_t)-3;
  return static_cast<uint64_t>(target->control.lifecycle.qos);
}
