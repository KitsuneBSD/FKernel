#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibC/string.h>

using namespace fkernel;

extern "C" {

uint64_t sys_stat(uint64_t path_ptr, uint64_t statbuf_ptr, uint64_t, uint64_t,
                  uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  if (!path_ptr || !statbuf_ptr)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  auto *current_task = SchedulerManager::the().current();
  if (!current_task)
    return fkernel::return_error(fk::core::Error::PermissionDenied);

  // Copy path from user space safely
  char path[512];
  strncpy(path, reinterpret_cast<const char *>(path_ptr), 511);
  path[511] = '\0';

  struct stat *buf = reinterpret_cast<struct stat *>(statbuf_ptr);

  char absolute_path[512];
  const char *final_path = path;

  if (path[0] != '/') {
    // Resolve relative path using CWD
    size_t cwd_len = strlen(current_task->files.cwd.c_str());
    if (cwd_len >= 512) return fkernel::return_error(fk::core::Error::IOError);
    
    strcpy(absolute_path, current_task->files.cwd.c_str());
    
    if (cwd_len > 0 && absolute_path[cwd_len - 1] != '/') {
      if (cwd_len + 1 < 512) {
          absolute_path[cwd_len] = '/';
          absolute_path[cwd_len + 1] = '\0';
          cwd_len++;
      }
    }
    
    if (cwd_len + strlen(path) < 512) {
      strcat(absolute_path, path);
    } else {
      return fkernel::return_error(fk::core::Error::IOError);
    }
    final_path = absolute_path;
  }

  // fk::algorithms::klog("SYSCALL", "sys_stat: path=%s", final_path);

  auto res = VirtualFileSystem::the().stat(final_path, buf);
  if (res.is_error()) {
    return fkernel::return_error(res.error());
  }

  return 0;
}
}
