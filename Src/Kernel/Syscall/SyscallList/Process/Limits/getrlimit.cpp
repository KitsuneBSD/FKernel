#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

extern "C" {
uint64_t sys_getrlimit([[maybe_unused]] uint64_t resource, uint64_t rlim_ptr, uint64_t, uint64_t,
                       uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  struct rlimit {
    uint64_t rlim_cur;
    uint64_t rlim_max;
  };

  auto* rlim = reinterpret_cast<struct rlimit*>(rlim_ptr);
  if (!rlim)
    return return_error(fk::core::Error::InvalidParameter);

  rlim->rlim_cur = 0x7fffffffffffffffUL;
  rlim->rlim_max = 0x7fffffffffffffffUL;

  return 0;
}
}
