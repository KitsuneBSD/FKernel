#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <LibFK/Utilities/memory.h>

extern "C" {

uint64_t sys_chdir(uint64_t path_ptr, uint64_t, uint64_t, uint64_t, uint64_t,
                   uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto *current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  if (!path_ptr || !fkernel::memory::is_user_address(path_ptr, 1))
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  const char *upath = reinterpret_cast<const char *>(path_ptr);
  size_t len = 0;
  while (len < 512 && upath[len] != '\0')
    len++;
  if (len >= 512)
    return fkernel::return_error(fk::core::Error::InvalidParameter);

  char kpath[512];
  kpath[0] = '\0';
  auto copy_res = fkernel::memory::copy_from_user(kpath, upath, len + 1);
  if (copy_res.is_error())
    return -14;

  auto res = fkernel::VirtualFileSystem::the().resolve_path(kpath);
  if (res.is_error())
    return fkernel::return_error(res.error());

  auto dentry = res.value();
  auto node = dentry->top_node();
  if (!node || !node->is_directory())
    return fkernel::return_error(fk::core::Error::NotADirectory);

  fk::text::String full_path = dentry->get_path();
  if (full_path.is_empty()) {
      current_task->resources.files.cwd.assign("/", 1);
  } else {
      current_task->resources.files.cwd.assign(full_path.c_str(), full_path.length());
  }

  return 0;
}
}