#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/error.h>

extern "C" {

uint64_t sys_dup(uint64_t oldfd_u64, uint64_t, uint64_t, uint64_t,
                 uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int oldfd = (int)oldfd_u64;
  auto* current = SchedulerManager::the().current();
  if (!current) return (uint64_t)-1;
  auto desc = current->get_file_descriptor(oldfd);
  if (!desc) return -static_cast<int>(fk::core::Error::InvalidHandle);
  auto& fds = current->resources.files.descriptors;
  for (size_t i = 0; i < fds.size(); ++i) {
    if (!fds[i]) {
      fds[i] = desc;
      return (uint64_t)i;
    }
  }
  return -static_cast<int>(fk::core::Error::OutOfMemory);
}

uint64_t sys_dup2(uint64_t oldfd_u64, uint64_t newfd_u64, uint64_t, uint64_t,
                  uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    int oldfd = (int)oldfd_u64;
    int newfd = (int)newfd_u64;

    auto* current = SchedulerManager::the().current();
    if (!current) return -1;

    auto desc = current->get_file_descriptor(oldfd);
    if (!desc) return -static_cast<int>(fk::core::Error::InvalidHandle);

    if (oldfd == newfd) return newfd;

    if (current->install_at(newfd, desc) < 0)
      return -static_cast<int>(fk::core::Error::InvalidParameter);
    return newfd;
}
}