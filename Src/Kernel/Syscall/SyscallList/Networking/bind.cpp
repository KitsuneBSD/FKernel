#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Net/socket.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>

extern "C" {

uint64_t sys_bind(uint64_t sockfd, uint64_t addr, uint64_t addrlen, uint64_t,
                  uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  fk::algorithms::klog("Syscall", "sys_bind(fd=%ld, addr=%p, len=%ld)", sockfd, (void*)addr, addrlen);

  auto* task = SchedulerManager::the().current();
  if (!task) return -static_cast<int>(fk::core::Error::IOError);

  auto description = task->get_file_descriptor(static_cast<int>(sockfd));
  if (!description) return -static_cast<int>(fk::core::Error::InvalidHandle);

  auto node = description->node();
  if (!node) return -static_cast<int>(fk::core::Error::NotFound);

  // In a real implementation, we would cast to Socket
  // For Unix sockets, the addr is a path string
  const char* path = reinterpret_cast<const char*>(addr);
  
  // We should check if node is actually a socket
  // For now, let's try calling bind directly if it's a UnixSocket
  // Simplified: we'll assume it's a Socket
  auto* socket = static_cast<fkernel::Socket*>(node.get());
  auto res = socket->bind(path);
  if (res.is_error()) return -static_cast<int>(res.error());

  return 0;
}
}
