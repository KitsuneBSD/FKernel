#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Net/socket.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>

extern "C" {

uint64_t sys_listen(uint64_t sockfd, uint64_t backlog, uint64_t, uint64_t,
                    uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  fk::algorithms::klog("Syscall", "sys_listen(fd=%ld, backlog=%ld)", sockfd, backlog);

  auto* task = SchedulerManager::the().current();
  if (!task) return -static_cast<int>(fk::core::Error::IOError);

  auto description = task->get_file_descriptor(static_cast<int>(sockfd));
  if (!description) return -static_cast<int>(fk::core::Error::InvalidHandle);

  auto node = description->node();
  auto* socket = static_cast<fkernel::Socket*>(node.get());
  
  auto res = socket->listen();
  if (res.is_error()) return -static_cast<int>(res.error());

  return 0;
}
}
