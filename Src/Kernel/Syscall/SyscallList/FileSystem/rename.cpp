#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibC/string.h>

using namespace fkernel;

extern "C" {
uint64_t sys_rename(uint64_t old_path_ptr, uint64_t new_path_ptr, uint64_t, uint64_t, uint64_t,
                    uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  const char* old_path = reinterpret_cast<const char*>(old_path_ptr);
  const char* new_path = reinterpret_cast<const char*>(new_path_ptr);
  if (!old_path || !new_path)
    return -static_cast<int>(fk::core::Error::InvalidParameter);

  const char* cwd = current_task->resources.files.cwd.c_str();

  char old_abs_path[512];
  char new_abs_path[512];

  if (old_path[0] == '/') {
    strncpy(old_abs_path, old_path, 511);
    old_abs_path[511] = '\0';
  } else {
    strcpy(old_abs_path, cwd);
    if (old_abs_path[0] != '\0' && old_abs_path[strlen(old_abs_path) - 1] != '/') {
      strcat(old_abs_path, "/");
    }
    strcat(old_abs_path, old_path);
  }

  if (new_path[0] == '/') {
    strncpy(new_abs_path, new_path, 511);
    new_abs_path[511] = '\0';
  } else {
    strcpy(new_abs_path, cwd);
    if (new_abs_path[0] != '\0' && new_abs_path[strlen(new_abs_path) - 1] != '/') {
      strcat(new_abs_path, "/");
    }
    strcat(new_abs_path, new_path);
  }

  auto res = VirtualFileSystem::the().rename(old_abs_path, new_abs_path);
  if (res.is_error())
    return return_error(res.error());

  return 0;
}
}
