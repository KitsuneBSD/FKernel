#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibC/string.h>

extern "C" {

uint64_t sys_chdir(uint64_t path_ptr, uint64_t, uint64_t, uint64_t, uint64_t,
                   uint64_t, PtRegs* regs) {
  auto *current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  const char *path = (const char *)path_ptr;
  if (!path)
    return -static_cast<int>(fk::core::Error::InvalidParameter);

  auto res = VirtualFileSystem::the().resolve_path(path);
  if (res.is_error())
    return -static_cast<int>(res.error());

  auto node = res.value();
  if (!node->is_directory())
    return -static_cast<int>(fk::core::Error::NotADirectory);

  fk::text::String full_path = node->get_path();
  if (full_path.is_empty()) {
      current_task->cwd.assign("/", 1);
  } else {
      current_task->cwd.assign(full_path.c_str(), full_path.length());
  }

  return 0;
}
}
