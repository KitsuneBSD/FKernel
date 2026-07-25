#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/file_description.h>
#include <Kernel/Net/socket.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/error.h>

extern "C" {

uint64_t sys_accept4(uint64_t sockfd, [[maybe_unused]] uint64_t addr,
                     [[maybe_unused]] uint64_t addrlen, uint64_t flags,
                     uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {

  auto* task = SchedulerManager::the().current();
  if (!task)
    return -static_cast<int>(fk::core::Error::IOError);

  auto description = task->get_file_descriptor(static_cast<int>(sockfd));
  if (!description)
    return -static_cast<int>(fk::core::Error::InvalidHandle);

  auto node = description->node();
  auto* socket = static_cast<fkernel::Socket*>(node.get());

  auto client_res = socket->accept();
  if (client_res.is_error())
    return -static_cast<int>(client_res.error());

  auto client_socket = client_res.value();
  auto client_dentry = fkernel::Dentry::create("socket_client", nullptr).value();
  client_dentry->push_node(client_socket);

  int fd_flags = O_RDWR;
  if (flags & 0x800) // SOCK_CLOEXEC
    fd_flags |= O_CLOEXEC;
  if (flags & 0x8000) // SOCK_NONBLOCK
    fd_flags |= O_NONBLOCK;

  auto client_desc = fk::make_ref<FileDescription>(client_dentry, fd_flags).value();
  int client_fd = task->add_file_descriptor(client_desc);

  return static_cast<uint64_t>(client_fd);
}
}
