#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/error.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/file_description.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Net/Sockets/socket.h>
#include <Kernel/Net/Sockets/unix_socket.h>
#include <Kernel/Net/Sockets/inet_socket.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" {

uint64_t sys_socket(uint64_t domain, uint64_t type, [[maybe_unused]] uint64_t protocol, uint64_t,
                    uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {

  fk::RefPtr<Node> socket_node;
  auto sock_domain = static_cast<fkernel::SocketDomain>(domain);
  auto sock_type = static_cast<fkernel::SocketType>(type);

  if (sock_domain == fkernel::SocketDomain::Unix) {
    auto socket_res = fkernel::UnixSocket::create(sock_type);
    if (socket_res.is_error())
      return -static_cast<int>(socket_res.error());
    socket_node = socket_res.value();
  } else if (sock_domain == fkernel::SocketDomain::Inet) {
    auto socket_res = fkernel::net::create_inet_socket(sock_type);
    if (socket_res.is_error())
      return -static_cast<int>(socket_res.error());
    socket_node = socket_res.value();
  } else {
    fk::algorithms::kwarn("SYSCALL", "sys_socket: unsupported domain %lu", domain);
    return -static_cast<int>(fk::core::Error::InvalidParameter);
  }

  // Create a virtual dentry for this socket (not bound to FS yet)
  auto dentry = fkernel::Dentry::create("socket", nullptr).value();
  dentry->push_node(socket_node);

  auto description = fk::make_ref<FileDescription>(dentry, O_RDWR).value();

  auto* task = SchedulerManager::the().current();
  if (!task)
    return -static_cast<int>(fk::core::Error::IOError);

  int fd = task->add_file_descriptor(description);

  return static_cast<uint64_t>(fd);
}
}
