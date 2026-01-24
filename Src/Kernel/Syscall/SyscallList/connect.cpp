#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>

extern "C" {

uint64_t sys_connect(uint64_t sockfd, uint64_t addr, uint64_t addrlen, uint64_t,
                     uint64_t, uint64_t) {
  fk::algorithms::kwarn("Syscall", "sys_connect(%ld, %p, %ld) not implemented",
                        sockfd, (void *)addr, addrlen);
  return -static_cast<int>(fk::core::Error::NotImplemented);
}
}
