#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <LibFK/Utilities/memory.h>
#include <Kernel/Fs/DebugFs/debug_fs.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/error.h>
#include <LibFK/Types/types.h>

extern "C" {
uint64_t sys_open(uint64_t path_ptr, uint64_t flags, uint64_t, uint64_t,
                  uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto *current_task = SchedulerManager::the().current();
  if (!current_task) {
    fk::algorithms::kwarn("SYSCALL", "sys_open: No current task");
    return fkernel::return_error(fk::core::Error::PermissionDenied);
  }

  if (!path_ptr || !fkernel::memory::is_user_address(path_ptr, 1))
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  // Copy path from user space into kernel buffer
  char kpath[512];
  kpath[0] = '\0';
  const char* upath = reinterpret_cast<const char*>(path_ptr);
  size_t len = 0;
  while (len < sizeof(kpath) - 1 && upath[len] != '\0')
    len++;
  if (len >= sizeof(kpath) - 1)
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  auto copy_res = fkernel::memory::copy_from_user(kpath, upath, len + 1);
  if (copy_res.is_error())
    return -14; // EFAULT

  const char *path = kpath;
  if (!path[0])
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  char absolute_path[512];
  if (path[0] != '/') {
      size_t cwd_len = fk::memory::length(current_task->resources.files.cwd.c_str());
      if (cwd_len >= 512) return fkernel::return_error(fk::core::Error::IOError);
      
      fk::memory::copy_string(absolute_path, current_task->resources.files.cwd.c_str());
      if (cwd_len > 0 && absolute_path[cwd_len-1] != '/') {
          if (cwd_len + 1 < 512) {
              fk::memory::concatenate(absolute_path, "/");
              cwd_len++;
          }
      }
      
      if (cwd_len + fk::memory::length(path) < 512) {
          fk::memory::concatenate(absolute_path, path);
      } else {
          return fkernel::return_error(fk::core::Error::IOError);
      }
      path = absolute_path;
  }

  // /dev/ptmx: allocate a new PTY master and return its fd directly
  if (fk::memory::compare(path, "/dev/ptmx") == 0) {
    int mfd = -1, sfd = -1;
    extern uint64_t sys_openpty(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, PtRegs*);
    uint64_t r = sys_openpty((uint64_t)&mfd, (uint64_t)&sfd, 0, 0, 0, 0, regs);
    if ((int64_t)r < 0) return r;
    // Close slave — caller obtains it via ptsname(mfd) + open
    if (sfd >= 0) current_task->close_file_descriptor(sfd);
    return (uint64_t)mfd;
  }

  auto result = fkernel::VirtualFileSystem::the().open(path, (int)flags);
  if (result.is_error()) {
    return fkernel::return_error(result.error());
  }

  int fd = current_task->add_file_descriptor(result.value());
  if (fd < 0) {
      return fkernel::return_error(fk::core::Error::DeviceError); // EMFILE
  }

  return fd;
}
}