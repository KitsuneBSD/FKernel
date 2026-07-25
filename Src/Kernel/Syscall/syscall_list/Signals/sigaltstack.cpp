#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

struct stack_t {
  void*  ss_sp;
  int    ss_flags;
  size_t ss_size;
};

static constexpr int SS_DISABLE = 2;
static constexpr int SS_ONSTACK = 1;

extern "C" uint64_t sys_sigaltstack(uint64_t new_ptr, uint64_t old_ptr,
                                     uint64_t, uint64_t, uint64_t, uint64_t,
                                     [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;

  auto& alt = task->ipc().alt_stack;

  // Report current alt stack state
  if (old_ptr) {
    auto* old = reinterpret_cast<stack_t*>(old_ptr);
    old->ss_sp    = alt.ss_sp;
    old->ss_flags = alt.ss_sp ? SS_ONSTACK : SS_DISABLE;
    old->ss_size  = alt.ss_size;
  }

  // Install new alt stack
  if (new_ptr) {
    auto* ns = reinterpret_cast<const stack_t*>(new_ptr);

    // If currently on alt stack, return EPERM
    uint64_t sp = regs->rsp;
    if (alt.ss_sp && sp >= (uint64_t)alt.ss_sp &&
        sp < (uint64_t)alt.ss_sp + alt.ss_size) {
      return (uint64_t)-1; // EPERM
    }

    if (ns->ss_flags == SS_DISABLE) {
      alt.ss_sp = nullptr;
      alt.ss_size = 0;
      alt.ss_flags = SS_DISABLE;
    } else {
      if (ns->ss_size < 512) return (uint64_t)-22; // EINVAL
      alt.ss_sp = ns->ss_sp;
      alt.ss_size = ns->ss_size;
      alt.ss_flags = SS_ONSTACK;
    }
  }

  return 0;
}
