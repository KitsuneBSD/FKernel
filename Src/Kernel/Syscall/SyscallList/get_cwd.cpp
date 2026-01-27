#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibC/string.h>

extern "C" {

uint64_t sys_getcwd(uint64_t buf_ptr, uint64_t size, uint64_t, uint64_t,
                    uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto *current_task = SchedulerManager::the().current();
  if (!current_task)
    return fkernel::return_error(fk::core::Error::PermissionDenied);

  if (!buf_ptr || size < current_task->cwd.size() + 1)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  char *buf = (char *)buf_ptr;
  strcpy(buf, current_task->cwd.c_str());

  return buf_ptr; // Linux returns buf pointer on success
}
}
