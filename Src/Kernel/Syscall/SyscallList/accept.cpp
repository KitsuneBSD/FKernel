#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>

extern "C" {
uint64_t sys_accept(uint64_t sockfd, uint64_t addr, uint64_t addrlen, uint64_t,
                    uint64_t, uint64_t, PtRegs* regs) {
  fk::algorithms::kwarn("Syscall", "sys_accept(%ld, %p, %p) not implemented",
                        sockfd, (void *)addr, (void *)addrlen);
  return -static_cast<int>(fk::core::Error::NotImplemented);
}
}
