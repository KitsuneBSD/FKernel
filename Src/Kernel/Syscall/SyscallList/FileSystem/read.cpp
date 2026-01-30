#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Types/types.h>

extern "C" {
uint64_t sys_read(uint64_t fd_u64, uint64_t buffer_ptr, uint64_t size,
                  uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int fd = (int)fd_u64;

  auto *current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  if (fd < 0 || (size_t)fd >= current_task->resources.files.descriptors.size()) {
    return -static_cast<int>(fk::core::Error::InvalidHandle);
  }

  auto &desc = current_task->resources.files.descriptors[fd];
  if (!desc) {
    return -static_cast<int>(fk::core::Error::InvalidHandle);
  }

  auto result = desc->read(size, reinterpret_cast<uint8_t *>(buffer_ptr));
  if (result.is_error()) {
    return -static_cast<int>(result.error());
  }

  return result.value();
}
}