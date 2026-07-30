#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

extern "C" {
uint64_t sys_mkdir(uint64_t path_ptr, uint64_t mode, uint64_t, uint64_t, uint64_t, uint64_t,
                   [[maybe_unused]] PtRegs* regs) {
  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  if (!path_ptr || !fkernel::memory::is_user_address(path_ptr, 1))
    return -static_cast<int>(fk::core::Error::InvalidParameter);

  char kpath[512];
  auto copy_res = fkernel::memory::copy_from_user(kpath, reinterpret_cast<const void*>(path_ptr), sizeof(kpath));
  if (copy_res.is_error())
    return -14;
  kpath[sizeof(kpath) - 1] = '\0';

  const char* path = kpath;

  char absolute_path[512];
  if (path[0] != '/') {
    const char* cwd = current_task->resources.files.cwd.c_str();
    size_t cwd_len = fk::memory::length(cwd);
    fk::memory::copy_string(absolute_path, cwd);

    if (cwd_len > 0 && absolute_path[cwd_len - 1] != '/') {
      fk::memory::concatenate(absolute_path, "/");
    }
    fk::memory::concatenate(absolute_path, path);
    path = absolute_path;
  }

  auto res = VirtualFileSystem::the().mkdir(path, (int)mode);
  if (res.is_error())
    return return_error(res.error());

  return 0;
}
}