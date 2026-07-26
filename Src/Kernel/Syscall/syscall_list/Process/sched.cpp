#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Scheduler/qos.h>
#include <Kernel/Scheduler/turnstile.h>
#include <Kernel/Syscall/syscall.h>

using namespace fkernel::scheduler;

static constexpr int SCHED_OTHER = 0;
static constexpr int SCHED_FIFO = 1;
static constexpr int SCHED_RR = 2;
static constexpr int SCHED_BATCH = 3;
static constexpr int SCHED_IDLE = 5;

struct SchedParam { int priority; };

static int policy_to_linux(SchedulingPolicy p) {
  switch (p) {
    case SchedulingPolicy::Fifo:       return SCHED_FIFO;
    case SchedulingPolicy::RoundRobin: return SCHED_RR;
    case SchedulingPolicy::Batch:      return SCHED_BATCH;
    case SchedulingPolicy::Idle:       return SCHED_IDLE;
    default:                           return SCHED_OTHER;
  }
}

static SchedulingPolicy linux_to_policy(int policy) {
  switch (policy) {
    case SCHED_FIFO:  return SchedulingPolicy::Fifo;
    case SCHED_RR:    return SchedulingPolicy::RoundRobin;
    case SCHED_BATCH: return SchedulingPolicy::Batch;
    case SCHED_IDLE:  return SchedulingPolicy::Idle;
    default:          return SchedulingPolicy::Normal;
  }
}

extern "C" {

uint64_t sys_sched_getscheduler(uint64_t pid, uint64_t, uint64_t,
                                uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto task = SchedulerManager::the().find_task(fk::ProcessId(pid));
  if (!task.get()) return (uint64_t)-3;
  return (uint64_t)policy_to_linux(task->control.lifecycle.policy);
}

uint64_t sys_sched_setscheduler(uint64_t pid, uint64_t policy, uint64_t param_ptr,
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

uint64_t sys_sched_getparam(uint64_t pid, uint64_t param_ptr, uint64_t,
                            uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto task = SchedulerManager::the().find_task(fk::ProcessId(pid));
  if (!task.get()) return (uint64_t)-3;
  if (param_ptr) {
    auto* p = reinterpret_cast<SchedParam*>(param_ptr);
    p->priority = task->control.lifecycle.priority;
  }
  return 0;
}

uint64_t sys_sched_setparam(uint64_t pid, uint64_t param_ptr, uint64_t,
                            uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto task = SchedulerManager::the().find_task(fk::ProcessId(pid));
  if (!task.get()) return (uint64_t)-3;
  if (param_ptr) {
    auto* p = reinterpret_cast<SchedParam*>(param_ptr);
    if (p->priority > 0 && p->priority <= 99)
      task->control.lifecycle.priority = (uint8_t)p->priority;
  }
  return 0;
}

uint64_t sys_sched_get_priority_max(uint64_t policy, uint64_t, uint64_t,
                                    uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int p = (int)policy;
  if (p == SCHED_FIFO || p == SCHED_RR) return 99;
  if (p == SCHED_OTHER || p == SCHED_BATCH || p == SCHED_IDLE) return 0;
  return (uint64_t)-22;
}

uint64_t sys_sched_get_priority_min(uint64_t policy, uint64_t, uint64_t,
                                    uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int p = (int)policy;
  if (p == SCHED_FIFO || p == SCHED_RR) return 1;
  if (p == SCHED_OTHER || p == SCHED_BATCH || p == SCHED_IDLE) return 0;
  return (uint64_t)-22;
}

}
