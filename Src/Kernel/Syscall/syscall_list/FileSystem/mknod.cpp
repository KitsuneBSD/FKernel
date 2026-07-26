#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

extern "C" {
uint64_t sys_mknod(uint64_t path_ptr, uint64_t mode,
                   uint64_t dev, uint64_t, uint64_t, uint64_t,
                   [[maybe_unused]] PtRegs* regs) {
  (void)dev;
  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return (uint64_t)-1;

  const char* path = reinterpret_cast<const char*>(path_ptr);
  if (!path)
    return (uint64_t)-22;

  char absolute_path[512];
  if (path[0] != '/') {
    const char* cwd = current_task->resources.files.cwd.c_str();
    size_t cwd_len = fk::memory::length(cwd);
    fk::memory::copy_string(absolute_path, cwd);
    if (cwd_len > 0 && absolute_path[cwd_len - 1] != '/')
      fk::memory::concatenate(absolute_path, "/");
    fk::memory::concatenate(absolute_path, path);
    path = absolute_path;
  }

  // S_IFREG and S_IFIFO supported
  uint32_t type = (uint32_t)mode & 0170000;
  if (type != 0 && type != 0100000 && type != 0010000)
    return (uint64_t)-22;

  if (type == 0010000) {
    return fkernel::VirtualFileSystem::the().mkfifo(path, (int)mode).is_error()
               ? (uint64_t)-1 : 0;
  }

  auto res = VirtualFileSystem::the().open(path, O_CREAT | O_WRONLY | O_TRUNC);
  if (res.is_error())
    return return_error(res.error());

  // Apply permissions if a mode was given
  if (type == 0100000 || mode != 0) {
    auto dentry_res = VirtualFileSystem::the().resolve_path(path);
    if (!dentry_res.is_error()) {
      auto node = dentry_res.value()->top_node();
      if (node) node->set_permissions((uint32_t)mode);
    }
  }

  return 0;
}
}
