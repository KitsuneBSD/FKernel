#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

extern "C" {
uint64_t sys_symlink(uint64_t target_ptr, uint64_t linkpath_ptr, uint64_t, uint64_t, uint64_t,
                     uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  const char* target = reinterpret_cast<const char*>(target_ptr);
  const char* linkpath = reinterpret_cast<const char*>(linkpath_ptr);
  if (!target || !linkpath)
    return return_error(fk::core::Error::InvalidParameter);

  char absolute_path[512];
  if (linkpath[0] != '/') {
    const char* cwd = current_task->resources.files.cwd.c_str();
    size_t cwd_len = fk::memory::length(cwd);
    fk::memory::copy_string(absolute_path, cwd);

    if (cwd_len > 0 && absolute_path[cwd_len - 1] != '/') {
      fk::memory::concatenate(absolute_path, "/");
    }
    fk::memory::concatenate(absolute_path, linkpath);
    linkpath = absolute_path;
  }

  auto res = VirtualFileSystem::the().symlink(linkpath, target);
  if (res.is_error())
    return return_error(res.error());

  return 0;
}
}
