#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/Qos/qos.h>
#include <Kernel/Scheduler/Sync/turnstile.h>
#include <Kernel/Syscall/syscall.h>

using namespace fkernel::scheduler;

static constexpr int SCHED_OTHER = 0;
static constexpr int SCHED_FIFO = 1;
static constexpr int SCHED_RR = 2;
static constexpr int SCHED_BATCH = 3;
static constexpr int SCHED_IDLE = 5;

static int policy_to_linux(SchedulingPolicy p) {
  switch (p) {
    case SchedulingPolicy::Fifo:       return SCHED_FIFO;
    case SchedulingPolicy::RoundRobin: return SCHED_RR;
    case SchedulingPolicy::Batch:      return SCHED_BATCH;
    case SchedulingPolicy::Idle:       return SCHED_IDLE;
    default:                           return SCHED_OTHER;
  }
}

// sys_sched_getscheduler(...) → 0 or -errno
extern "C" uint64_t sys_sched_getscheduler(uint64_t pid, uint64_t, uint64_t,
                                uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto task = SchedulerManager::the().find_task(fk::ProcessId(pid));
  if (!task.get()) return (uint64_t)-3;
  return (uint64_t)policy_to_linux(task->control.lifecycle.policy);
}
