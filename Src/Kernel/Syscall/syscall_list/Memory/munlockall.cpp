#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

// sys_munlockall() → 0 or -errno
// No swap: nothing to unlock.
extern "C" uint64_t sys_munlockall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                      [[maybe_unused]] PtRegs* regs) {
  return 0;
}
