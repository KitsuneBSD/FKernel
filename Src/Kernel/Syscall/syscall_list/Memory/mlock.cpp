#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" {
uint64_t sys_mlock(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                   [[maybe_unused]] PtRegs* regs) {
  return fkernel::return_error(fk::core::Error::NotImplemented);
}
uint64_t sys_munlock(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                     [[maybe_unused]] PtRegs* regs) {
  return fkernel::return_error(fk::core::Error::NotImplemented);
}
uint64_t sys_mlockall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                      [[maybe_unused]] PtRegs* regs) {
  return fkernel::return_error(fk::core::Error::NotImplemented);
}
uint64_t sys_munlockall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                        [[maybe_unused]] PtRegs* regs) {
  return fkernel::return_error(fk::core::Error::NotImplemented);
}
}
