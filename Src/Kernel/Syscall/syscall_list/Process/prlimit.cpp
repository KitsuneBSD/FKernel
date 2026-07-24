#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

struct RLimit64 {
  uint64_t rlim_cur;
  uint64_t rlim_max;
};

static constexpr uint64_t RLIM_INFINITY = 0x7fffffffffffffffUL;

extern "C" {

uint64_t sys_prlimit64(uint64_t pid, uint64_t resource, uint64_t new_limit_ptr,
                       uint64_t old_limit_ptr, uint64_t, uint64_t,
                       [[maybe_unused]] PtRegs* regs) {
  (void)pid; (void)resource; (void)new_limit_ptr;
  if (old_limit_ptr) {
    auto* rlim = reinterpret_cast<RLimit64*>(old_limit_ptr);
    rlim->rlim_cur = RLIM_INFINITY;
    rlim->rlim_max = RLIM_INFINITY;
  }
  return 0;
}

}
