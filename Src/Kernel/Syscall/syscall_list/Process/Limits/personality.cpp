#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall_utils.h>

// SYS_PERSONALITY = 135 — musl ld.so calls this on startup.
// Return 0 (PER_LINUX) to indicate Linux personality.
extern "C" uint64_t sys_personality(uint64_t persona, uint64_t, uint64_t,
                                    uint64_t, uint64_t, uint64_t,
                                    [[maybe_unused]] PtRegs* regs) {
  (void)persona;
  return 0;
}
