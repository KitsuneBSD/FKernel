#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>

// madvise — performance hints; safe to ignore in FKernel
extern "C" uint64_t sys_madvise(uint64_t, uint64_t, uint64_t,
                                 uint64_t, uint64_t, uint64_t,
                                 [[maybe_unused]] PtRegs* regs) {
  return 0;
}
