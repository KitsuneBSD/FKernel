#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

extern "C" {
uint64_t sys_unlink(uint64_t path_ptr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                    [[maybe_unused]] PtRegs* regs) {
  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  const char* path = reinterpret_cast<const char*>(path_ptr);
  if (!path)
    return -static_cast<int>(fk::core::Error::InvalidParameter);

  char absolute_path[512];
  if (path[0] != '/') {
    const char* cwd = current_task->resources.files.cwd.c_str();
    size_t cwd_len = strlen(cwd);
    strcpy(absolute_path, cwd);

    if (cwd_len > 0 && absolute_path[cwd_len - 1] != '/') {
      strcat(absolute_path, "/");
    }
    strcat(absolute_path, path);
    path = absolute_path;
  }

  auto res = VirtualFileSystem::the().unlink(path);
  if (res.is_error())
    return return_error(res.error());

  return 0;
}
}
