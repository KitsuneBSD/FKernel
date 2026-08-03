#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

extern "C" {

uint64_t sys_stat(uint64_t path_ptr, uint64_t statbuf_ptr, uint64_t, uint64_t,
                  uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  if (!path_ptr || !statbuf_ptr)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  if (!fkernel::memory::is_user_address(path_ptr, 1))
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  if (!fkernel::memory::is_user_address(statbuf_ptr, sizeof(struct stat)))
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  auto *current_task = SchedulerManager::the().current();
  if (!current_task)
    return fkernel::return_error(fk::core::Error::PermissionDenied);

  char path[512];
  auto path_copy = fkernel::memory::copy_from_user(path, reinterpret_cast<const void*>(path_ptr), sizeof(path));
  if (path_copy.is_error())
    return -14;
  path[sizeof(path) - 1] = '\0';

  struct stat kbuf;
  fk::memory::set(&kbuf, 0, sizeof(kbuf));

  char absolute_path[512];
  const char *final_path = path;

  if (path[0] != '/') {
    size_t cwd_len = fk::memory::length(current_task->resources.files.cwd.c_str());
    if (cwd_len >= 512) return fkernel::return_error(fk::core::Error::IOError);
    
    fk::memory::copy_string(absolute_path, current_task->resources.files.cwd.c_str());
    
    if (cwd_len > 0 && absolute_path[cwd_len - 1] != '/') {
      if (cwd_len + 1 < 512) {
          absolute_path[cwd_len] = '/';
          absolute_path[cwd_len + 1] = '\0';
          cwd_len++;
      }
    }
    
    if (cwd_len + fk::memory::length(path) < 512) {
      fk::memory::concatenate(absolute_path, path);
    } else {
      return fkernel::return_error(fk::core::Error::IOError);
    }
    final_path = absolute_path;
  }

  auto res = VirtualFileSystem::the().stat(final_path, &kbuf);
  if (res.is_error()) {
    return fkernel::return_error(res.error());
  }

  auto copy_out = fkernel::memory::copy_to_user(
      reinterpret_cast<void *>(statbuf_ptr), &kbuf, sizeof(struct stat));
  if (copy_out.is_error())
    return -14;

  return 0;
}
}