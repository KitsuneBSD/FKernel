#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Net/unix_socket.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/file_description.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>

extern "C" {

uint64_t sys_socket(uint64_t domain, uint64_t type, uint64_t protocol, uint64_t,
                    uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  fk::algorithms::klog("Syscall", "sys_socket(domain=%ld, type=%ld, protocol=%ld)",
                        domain, type, protocol);

  fk::RefPtr<Node> socket_node;

  if (domain == 1) { // AF_UNIX / AF_LOCAL
      auto socket_res = fkernel::UnixSocket::create(static_cast<fkernel::SocketType>(type));
      if (socket_res.is_error()) return -static_cast<int>(socket_res.error());
      socket_node = socket_res.value();
  } else {
      // Fallback legacy behavior: return eth0
      auto dentry_res = fkernel::VirtualFileSystem::the().resolve_path("/dev/eth0");
      if (dentry_res.is_error()) {
          fk::algorithms::kerror("Syscall", "Failed to find /dev/eth0 for generic socket");
          return -static_cast<int>(fk::core::Error::NotFound);
      }
      socket_node = dentry_res.value()->top_node();
  }

  // Create a virtual dentry for this socket (not bound to FS yet)
  auto dentry = fkernel::Dentry::create("socket", nullptr).value();
  dentry->push_node(socket_node);

  auto description = fk::make_ref<FileDescription>(dentry, O_RDWR).value();

  auto* task = SchedulerManager::the().current();
  if (!task) return -static_cast<int>(fk::core::Error::IOError);

  int fd = task->add_file_descriptor(description);
  fk::algorithms::klog("Syscall", "sys_socket: created fd %d (domain %ld)", fd, domain);

  return static_cast<uint64_t>(fd);
}
}
