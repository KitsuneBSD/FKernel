#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>

extern "C" {

uint64_t sys_socket(uint64_t domain, uint64_t type, uint64_t protocol, uint64_t,
                    uint64_t, uint64_t, PtRegs* regs) {
  fk::algorithms::kwarn("Syscall", "sys_socket(%ld, %ld, %ld) not implemented",
                        domain, type, protocol);
  return -static_cast<int>(fk::core::Error::NotImplemented);
}
}
