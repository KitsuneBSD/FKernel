#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>

extern "C" {

uint64_t sys_listen(uint64_t sockfd, uint64_t backlog, uint64_t, uint64_t,
                    uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  fk::algorithms::kwarn("Syscall", "sys_listen(%ld, %ld) not implemented",
                        sockfd, backlog);
  return -static_cast<int>(fk::core::Error::NotImplemented);
}
}
