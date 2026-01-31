#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Net/socket.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/file_description.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>

extern "C" {

uint64_t sys_accept(uint64_t sockfd, [[maybe_unused]] uint64_t addr, [[maybe_unused]] uint64_t addrlen, uint64_t,
                    uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  fk::algorithms::klog("Syscall", "sys_accept(fd=%ld)", sockfd);

  auto* task = SchedulerManager::the().current();
  if (!task) return -static_cast<int>(fk::core::Error::IOError);

  auto description = task->get_file_descriptor(static_cast<int>(sockfd));
  if (!description) return -static_cast<int>(fk::core::Error::InvalidHandle);

  auto node = description->node();
  auto* socket = static_cast<fkernel::Socket*>(node.get());
  
  auto client_res = socket->accept();
  if (client_res.is_error()) return -static_cast<int>(client_res.error());

  auto client_socket = client_res.value();
  auto client_dentry = fkernel::Dentry::create("socket_client", nullptr).value();
  client_dentry->push_node(client_socket);

  auto client_desc = fk::make_ref<FileDescription>(client_dentry, O_RDWR).value();
  int client_fd = task->add_file_descriptor(client_desc);

  fk::algorithms::klog("Syscall", "sys_accept: created client fd %d", client_fd);
  return static_cast<uint64_t>(client_fd);
}
}
