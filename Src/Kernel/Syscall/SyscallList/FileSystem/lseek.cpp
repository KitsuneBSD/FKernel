#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Types/types.h>

extern "C" {
uint64_t sys_lseek(uint64_t fd_u64, uint64_t offset, uint64_t whence,
                   uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int fd = (int)fd_u64;

  auto *current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  if (fd < 0 || fd >= (int)current_task->resources.files.descriptors.size())
    return -static_cast<int>(fk::core::Error::InvalidHandle);

  auto &desc = current_task->resources.files.descriptors[fd];
  if (!desc) {
    return -static_cast<int>(fk::core::Error::InvalidHandle);
  }

  auto res = desc->seek(offset, static_cast<SeekMode>(whence));
  if (res.is_error()) {
    return -static_cast<int>(res.error());
  }

  return res.value();
}
}