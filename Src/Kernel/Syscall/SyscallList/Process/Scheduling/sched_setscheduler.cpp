#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <Kernel/Scheduler/Sync/turnstile.h>
#include <Kernel/Syscall/syscall.h>

using namespace fkernel::scheduler;

static constexpr int SCHED_FIFO = 1;
static constexpr int SCHED_RR = 2;
static constexpr int SCHED_BATCH = 3;
static constexpr int SCHED_IDLE = 5;

struct SchedParam { int priority; };

static SchedulingPolicy linux_to_policy(int policy) {
  switch (policy) {
    case SCHED_FIFO:  return SchedulingPolicy::Fifo;
    case SCHED_RR:    return SchedulingPolicy::RoundRobin;
    case SCHED_BATCH: return SchedulingPolicy::Batch;
    case SCHED_IDLE:  return SchedulingPolicy::Idle;
    default:          return SchedulingPolicy::Normal;
  }
}

// sys_sched_setscheduler(...) → 0 or -errno
extern "C" uint64_t sys_sched_setscheduler(uint64_t pid, uint64_t policy, uint64_t param_ptr,
                                uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto task = SchedulerManager::the().find_task(fk::ProcessId(pid));
  if (!task.get()) return (uint64_t)-3;

  task->control.lifecycle.policy = linux_to_policy((int)policy);

  if (param_ptr) {
    auto* p = reinterpret_cast<SchedParam*>(param_ptr);
    if (p->priority > 0 && p->priority <= 99) {
      task->control.lifecycle.priority = (uint8_t)p->priority;
    }
  }
  return 0;
}
