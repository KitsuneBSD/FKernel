#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibC/string.h>

extern "C" {

uint64_t sys_chdir(uint64_t path_ptr, uint64_t, uint64_t, uint64_t, uint64_t,
                   uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto *current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  const char *path = (const char *)path_ptr;
  if (!path)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  auto res = fkernel::VirtualFileSystem::the().resolve_path(path);
  if (res.is_error())
    return fkernel::return_error(res.error());

  auto dentry = res.value();
  auto node = dentry->top_node();
  if (!node || !node->is_directory())
    return fkernel::return_error(fk::core::Error::NotADirectory);

  fk::text::String full_path = dentry->get_path();
  if (full_path.is_empty()) {
      current_task->cwd.assign("/", 1);
  } else {
      current_task->cwd.assign(full_path.c_str(), full_path.length());
  }

  return 0;
}
}
