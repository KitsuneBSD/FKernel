#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Types/types.h>

extern "C" {
uint64_t sys_read(uint64_t fd_u64, uint64_t buf_ptr, uint64_t count, uint64_t,
                  uint64_t, uint64_t) {
  int fd = (int)fd_u64;
  fk::algorithms::klog("SYSCALL", "sys_read: start (fd=%d, count=%zu)", fd, (size_t)count);

  auto *current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  if (fd < 0 || (size_t)fd >= current_task->file_descriptors.size()) {
    fk::algorithms::kwarn("SYSCALL", "sys_read: FD %d out of range (size: %zu)",
                          fd, current_task->file_descriptors.size());
    return -static_cast<int>(fk::core::Error::InvalidHandle);
  }

  auto &desc = current_task->file_descriptors[fd];
  if (!desc) {
    fk::algorithms::kwarn("SYSCALL", "sys_read: FD %d exists but is NULL", fd);
    return -static_cast<int>(fk::core::Error::InvalidHandle);
  }

  // fk::algorithms::klog("SYSCALL", "sys_read: calling desc->read (desc ptr: %p)", desc.get());
  auto res = desc->read((size_t)count, (uint8_t *)buf_ptr);
  // fk::algorithms::klog("SYSCALL", "sys_read: return from desc->read");
  if (res.is_error()) {
    fk::algorithms::kwarn("SYSCALL", "sys_read: failed with error %d",
                          (int)res.error());
    return -static_cast<int>(res.error());
  }

  // fk::algorithms::kdebug("Syscall", "sys_read: success read %zu bytes",
  // res.value());
  // fk::algorithms::klog("SYSCALL", "sys_read: end (read %zu bytes)", res.value());
  return res.value();
}
}
