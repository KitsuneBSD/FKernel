#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" uint64_t sys_msync(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                [[maybe_unused]] PtRegs* regs) {
  return fkernel::return_error(fk::core::Error::NotImplemented);
}
