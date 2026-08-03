#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <LibFK/Utilities/memory.h>

extern "C" {

uint64_t sys_getcwd(uint64_t buf_ptr, uint64_t size, uint64_t, uint64_t,
                    uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto *current_task = SchedulerManager::the().current();
  if (!current_task)
    return fkernel::return_error(fk::core::Error::PermissionDenied);

  if (!buf_ptr || size < current_task->resources.files.cwd.size() + 1)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  char *buf = (char *)buf_ptr;
  fk::memory::copy_string(buf, current_task->resources.files.cwd.c_str());

  // Linux getcwd(2) returns the path length (including null), not the pointer.
  // The fk_user.h wrapper captures this as int, so returning the pointer (> 2GB)
  // would truncate to a negative int and fail the r >= 0 check in userspace.
  return current_task->resources.files.cwd.size() + 1;
}
}