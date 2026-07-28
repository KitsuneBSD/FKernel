#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/error.h>
#include <LibFK/Types/types.h>

extern "C" {
uint64_t sys_close(uint64_t fd_u64, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                   [[maybe_unused]] PtRegs* regs) {
  int fd = (int)fd_u64;

  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  if (fd < 0 || fd >= (int)current_task->resources.files.descriptors.size())
    return -static_cast<int>(fk::core::Error::InvalidHandle);

  if (!current_task->resources.files.descriptors[fd]) {
    fk::algorithms::kwarn("SYSCALL", "sys_close: fd %d not open", fd);
    return -static_cast<int>(fk::core::Error::InvalidHandle);
  }

  current_task->close_file_descriptor(fd);
  return 0;
}
}
