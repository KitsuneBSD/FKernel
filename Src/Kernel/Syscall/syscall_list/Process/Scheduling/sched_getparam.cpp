#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <Kernel/Scheduler/Sync/turnstile.h>
#include <Kernel/Syscall/syscall.h>

using namespace fkernel::scheduler;

struct SchedParam { int priority; };

// sys_sched_getparam(...) → 0 or -errno
extern "C" uint64_t sys_sched_getparam(uint64_t pid, uint64_t param_ptr, uint64_t,
                            uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto task = SchedulerManager::the().find_task(fk::ProcessId(pid));
  if (!task.get()) return (uint64_t)-3;
  if (param_ptr) {
    auto* p = reinterpret_cast<SchedParam*>(param_ptr);
    p->priority = task->control.lifecycle.priority;
  }
  return 0;
}
