#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/error.h>
#include <LibFK/Types/types.h>

// sys_ftruncate(...) → 0 or -errno
extern "C" uint64_t sys_ftruncate(uint64_t fd_u64, uint64_t length, uint64_t,
                       uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  int fd = (int)fd_u64;

  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  if (fd < 0 || fd >= (int)current_task->resources.files.descriptors.size())
    return -static_cast<int>(fk::core::Error::InvalidHandle);

  auto& desc = current_task->resources.files.descriptors[fd];
  if (!desc)
    return -static_cast<int>(fk::core::Error::InvalidHandle);

  auto node = desc->node();
  if (!node)
    return -static_cast<int>(fk::core::Error::InvalidHandle);

  auto res = node->truncate(length);
  if (res.is_error())
    return -static_cast<int>(res.error());

  return 0;
}
