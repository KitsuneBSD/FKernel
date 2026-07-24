#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Utilities/memory.h>

// SCHED_OTHER = 0, SCHED_FIFO = 1, SCHED_RR = 2
static constexpr int SCHED_OTHER = 0;
static constexpr int SCHED_FIFO = 1;
static constexpr int SCHED_RR = 2;
static constexpr int SCHED_BATCH = 3;
static constexpr int SCHED_IDLE = 5;

// struct sched_param { int sched_priority; };
struct SchedParam { int priority; };

extern "C" {

uint64_t sys_sched_getscheduler(uint64_t pid, uint64_t, uint64_t,
                                uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  (void)pid;
  return SCHED_OTHER;
}

uint64_t sys_sched_setscheduler(uint64_t pid, uint64_t policy, uint64_t param_ptr,
                                uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  (void)pid; (void)policy; (void)param_ptr;
  return 0;
}

uint64_t sys_sched_getparam(uint64_t pid, uint64_t param_ptr, uint64_t,
                            uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  (void)pid;
  if (param_ptr) {
    auto* p = reinterpret_cast<SchedParam*>(param_ptr);
    p->priority = 0;
  }
  return 0;
}

uint64_t sys_sched_setparam(uint64_t pid, uint64_t param_ptr, uint64_t,
                            uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  (void)pid; (void)param_ptr;
  return 0;
}

uint64_t sys_sched_get_priority_max(uint64_t policy, uint64_t, uint64_t,
                                    uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int p = (int)policy;
  if (p == SCHED_FIFO || p == SCHED_RR) return 99;
  if (p == SCHED_OTHER || p == SCHED_BATCH || p == SCHED_IDLE) return 0;
  return (uint64_t)-22; // EINVAL
}

uint64_t sys_sched_get_priority_min(uint64_t policy, uint64_t, uint64_t,
                                    uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int p = (int)policy;
  if (p == SCHED_FIFO || p == SCHED_RR) return 1;
  if (p == SCHED_OTHER || p == SCHED_BATCH || p == SCHED_IDLE) return 0;
  return (uint64_t)-22; // EINVAL
}

}
