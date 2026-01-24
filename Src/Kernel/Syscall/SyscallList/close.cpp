#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Types/types.h>

extern "C" {
uint64_t sys_close(uint64_t fd_u64, uint64_t, uint64_t, uint64_t, uint64_t,
                   uint64_t) {
  int fd = (int)fd_u64;

  auto *current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  fk::algorithms::kdebug("SYSCALL", "sys_close: fd=%d", fd);

  if (fd < 0 || fd >= (int)current_task->file_descriptors.size())
    return -static_cast<int>(fk::core::Error::InvalidHandle);

  if (!current_task->file_descriptors[fd]) {
    fk::algorithms::kwarn("SYSCALL", "sys_close: fd %d not open", fd);
    return -static_cast<int>(fk::core::Error::InvalidHandle);
  }

  auto desc = current_task->file_descriptors[fd];
  fk::algorithms::klog("SYSCALL", "sys_close: fd=%d, desc=%p", fd, desc.get());

  current_task->file_descriptors[fd] = nullptr;
  fk::algorithms::klog("SYSCALL", "sys_close: success");
  return 0;
}
}
