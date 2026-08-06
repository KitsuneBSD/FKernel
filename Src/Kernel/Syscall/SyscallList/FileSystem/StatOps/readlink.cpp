#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

extern "C" {
uint64_t sys_readlink(uint64_t path_ptr, uint64_t buf_ptr, uint64_t bufsiz, uint64_t, uint64_t,
                      uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  if (!path_ptr || !fkernel::memory::is_user_address(path_ptr, 1)) return -14;
  if (!buf_ptr || bufsiz == 0 || !fkernel::memory::is_user_address(buf_ptr, bufsiz)) return -14;

  char kpath[512];
  const char* upath = reinterpret_cast<const char*>(path_ptr);
  size_t path_len = 0;
  while (path_len < sizeof(kpath) - 1 && upath[path_len] != '\0') ++path_len;
  auto path_copy = fkernel::memory::copy_from_user(kpath, upath, path_len + 1);
  if (path_copy.is_error()) return -14;

  char absolute_path[512];
  const char* path = kpath;
  if (kpath[0] != '/') {
    const char* cwd = current_task->resources.files.cwd.c_str();
    size_t cwd_len = fk::memory::length(cwd);
    if (cwd_len >= sizeof(absolute_path) - 1) return -36; // ENAMETOOLONG
    fk::memory::copy_n(absolute_path, cwd, sizeof(absolute_path) - 1);
    absolute_path[sizeof(absolute_path) - 1] = '\0';
    if (cwd_len > 0 && absolute_path[cwd_len - 1] != '/') {
      if (cwd_len + 1 >= sizeof(absolute_path)) return -36;
      absolute_path[cwd_len] = '/';
      absolute_path[cwd_len + 1] = '\0';
      ++cwd_len;
    }
    size_t remaining = sizeof(absolute_path) - cwd_len - 1;
    fk::memory::concatenate_n(absolute_path, kpath, remaining);
    path = absolute_path;
  }

  auto dentry_res = VirtualFileSystem::the().resolve_path(path);
  if (dentry_res.is_error())
    return return_error(fk::core::Error::NotFound);

  auto node = dentry_res.value()->top_node();
  if (!node || !node->is_symlink())
    return return_error(fk::core::Error::NotASymlink);

  auto link_res = node->read_link();
  if (link_res.is_error())
    return return_error(link_res.error());

  auto& target = link_res.value();
  size_t copy_len = target.length();
  if (copy_len >= bufsiz)
    copy_len = bufsiz - 1;

  auto out_res = fkernel::memory::copy_to_user(reinterpret_cast<void*>(buf_ptr), target.c_str(), copy_len);
  if (out_res.is_error()) return -14;

  char nul = '\0';
  fkernel::memory::copy_to_user(reinterpret_cast<void*>(buf_ptr + copy_len), &nul, 1);

  return copy_len;
}
}
