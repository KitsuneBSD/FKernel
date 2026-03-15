#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibC/string.h>

using namespace fkernel;

extern "C" {
uint64_t sys_readlink(uint64_t path_ptr, uint64_t buf_ptr, uint64_t bufsiz, uint64_t, uint64_t,
                      uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  const char* path = reinterpret_cast<const char*>(path_ptr);
  char* buf = reinterpret_cast<char*>(buf_ptr);
  if (!path || !buf)
    return return_error(fk::core::Error::InvalidParameter);

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

  auto dentry_res = VirtualFileSystem::the().resolve_path(path);
  if (dentry_res.is_error()) {
    return return_error(fk::core::Error::NotFound);
  }

  auto node = dentry_res.value()->top_node();
  if (!node || !node->is_symlink()) {
    return return_error(fk::core::Error::NotASymlink);
  }

  auto link_res = node->read_link();
  if (link_res.is_error()) {
    return return_error(link_res.error());
  }

  auto& target = link_res.value();
  size_t copy_len = target.length();
  if (copy_len >= bufsiz) {
    copy_len = bufsiz - 1;
  }

  memcpy(buf, target.c_str(), copy_len);
  buf[copy_len] = '\0';

  return copy_len;
}
}
