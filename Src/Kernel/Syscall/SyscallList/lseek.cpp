#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Types/types.h>

extern "C" {
uint64_t sys_lseek(uint64_t fd_u64, uint64_t offset, uint64_t whence, uint64_t,
                   uint64_t, uint64_t, PtRegs* regs) {
  int fd = (int)fd_u64;

  auto *current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  if (fd < 0 || fd >= (int)current_task->file_descriptors.size())
    return -static_cast<int>(fk::core::Error::InvalidHandle);

  auto &desc = current_task->file_descriptors[fd];
  if (!desc)
    return -static_cast<int>(fk::core::Error::InvalidHandle);

  SeekMode mode = SeekMode::Set;
  if (whence == 1)
    mode = SeekMode::Current;
  else if (whence == 2)
    mode = SeekMode::End;

  auto res = desc->seek(offset, mode);
  if (res.is_error()) {
    fk::algorithms::kwarn("Syscall", "sys_lseek: failed with error %d",
                          (int)res.error());
    return -static_cast<int>(res.error());
  }

  // fk::algorithms::kdebug("Syscall", "sys_lseek: success new offset %lu",
  // res.value());
  return res.value();
}
}
