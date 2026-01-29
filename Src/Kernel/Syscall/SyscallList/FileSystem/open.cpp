#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/DebugFs/debug_fs.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Types/types.h>

extern "C" {
uint64_t sys_open(uint64_t path_ptr, uint64_t flags, uint64_t, uint64_t,
                  uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto *current_task = SchedulerManager::the().current();
  if (!current_task) {
    fk::algorithms::kerror("Syscall", "sys_open: No current task");
    return fkernel::return_error(fk::core::Error::PermissionDenied);
  }

  const char *path = (const char *)path_ptr;
  // Log to DebugFS instead of console
  if (MemoryManager::the().is_heap_initialized()) {
      char log_buf[256];
      int log_len = snprintf(log_buf, sizeof(log_buf), "[SYSCALL] sys_open: path=%s flags=%lx\n", path, (uint64_t)flags);
      auto syscall_log = fkernel::SyscallLogNode::the();
      if (syscall_log) syscall_log->append(log_buf, log_len);
  }


  if (!path)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  char absolute_path[512];
  if (path[0] != '/') {
      size_t cwd_len = strlen(current_task->cwd.c_str());
      if (cwd_len >= 512) return fkernel::return_error(fk::core::Error::IOError);
      
      strcpy(absolute_path, current_task->cwd.c_str());
      if (cwd_len > 0 && absolute_path[cwd_len-1] != '/') {
          if (cwd_len + 1 < 512) {
              strcat(absolute_path, "/");
              cwd_len++;
          }
      }
      
      if (cwd_len + strlen(path) < 512) {
          strcat(absolute_path, path);
      } else {
          return fkernel::return_error(fk::core::Error::IOError);
      }
      path = absolute_path;
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
