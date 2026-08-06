#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

// sys_mlockall(flags) → 0 or -errno
// FKernel has no swap: all mapped pages are already resident, so locking all
// is a no-op.
extern "C" uint64_t sys_mlockall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                      [[maybe_unused]] PtRegs* regs) {
  return 0;
}
