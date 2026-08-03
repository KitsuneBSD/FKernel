#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Utilities/memory.h>
#include <Kernel/Fs/Virtual/DebugFs/debug_fs.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/error.h>
#include <LibFK/Types/types.h>

static bool open_check_permission(uint32_t file_mode, uint32_t file_uid, uint32_t file_gid,
                                   uint32_t caller_euid, uint32_t caller_egid,
                                   const uint32_t* supp_gids, uint32_t ngroups,
                                   int access_mode) {
    if (caller_euid == 0) {
        if (access_mode & 1) return (file_mode & 0111) != 0;
        return true;
    }
    if (caller_euid == file_uid) {
        uint32_t perm_bits = (file_mode >> 6) & 7;
        if ((access_mode & 4) && !(perm_bits & 4)) return false;
        if ((access_mode & 2) && !(perm_bits & 2)) return false;
        if ((access_mode & 1) && !(perm_bits & 1)) return false;
        return true;
    }
    bool in_group = (caller_egid == file_gid);
    if (!in_group) {
        for (uint32_t i = 0; i < ngroups && i < 16; ++i) {
            if (supp_gids[i] == file_gid) { in_group = true; break; }
        }
    }
    uint32_t perm_bits = in_group ? ((file_mode >> 3) & 7) : (file_mode & 7);
    if ((access_mode & 4) && !(perm_bits & 4)) return false;
    if ((access_mode & 2) && !(perm_bits & 2)) return false;
    if ((access_mode & 1) && !(perm_bits & 1)) return false;
    return true;
}

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
    fk::algorithms::kwarn("OPEN", "%s -> err %d", path, (int)result.error());
    return fkernel::return_error(result.error());
  }

  auto node = result.value()->node();
  if (node) {
    int acc = 0;
    int open_flags = (int)flags & 3;
    if (open_flags == 0)       acc = 4;        // O_RDONLY → R_OK
    else if (open_flags == 1) acc = 2;          // O_WRONLY → W_OK
    else if (open_flags == 2) acc = 4 | 2;      // O_RDWR → R_OK | W_OK

    auto* task = SchedulerManager::the().current();
    if (task && !open_check_permission(
            node->node_mode(), node->node_uid(), node->node_gid(),
            task->control.identity.euid, task->control.identity.egid,
            task->control.identity.supplementary_gids,
            task->control.identity.ngroups, acc)) {
      return fkernel::return_error(fk::core::Error::PermissionDenied);
    }
  }

  int fd = current_task->add_file_descriptor(result.value());
  if (fd < 0) {
      return fkernel::return_error(fk::core::Error::DeviceError);
  }

  return fd;
}
}