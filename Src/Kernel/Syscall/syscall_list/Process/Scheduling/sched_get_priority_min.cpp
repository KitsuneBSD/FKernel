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

// sys_sched_get_priority_min(...) → 0 or -errno
extern "C" uint64_t sys_sched_get_priority_min(uint64_t policy, uint64_t, uint64_t,
                                    uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int p = (int)policy;
  if (p == SCHED_FIFO || p == SCHED_RR) return 1;
  if (p == SCHED_OTHER || p == SCHED_BATCH || p == SCHED_IDLE) return 0;
  return (uint64_t)-22;
}
