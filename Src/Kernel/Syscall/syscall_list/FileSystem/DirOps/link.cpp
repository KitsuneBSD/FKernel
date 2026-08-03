#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

extern "C" {
uint64_t sys_link(uint64_t old_path_ptr, uint64_t new_path_ptr, uint64_t, uint64_t, uint64_t,
                  uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  const char* old_path = reinterpret_cast<const char*>(old_path_ptr);
  const char* new_path = reinterpret_cast<const char*>(new_path_ptr);
  if (!old_path || !new_path)
    return -static_cast<int>(fk::core::Error::InvalidParameter);

  const char* cwd = current_task->resources.files.cwd.c_str();

  char old_abs_path[512];
  char new_abs_path[512];

  if (old_path[0] == '/') {
    fk::memory::copy_n(old_abs_path, old_path, 511);
    old_abs_path[511] = '\0';
  } else {
    fk::memory::copy_string(old_abs_path, cwd);
    if (old_abs_path[0] != '\0' && old_abs_path[fk::memory::length(old_abs_path) - 1] != '/') {
      fk::memory::concatenate(old_abs_path, "/");
    }
    fk::memory::concatenate(old_abs_path, old_path);
  }

  if (new_path[0] == '/') {
    fk::memory::copy_n(new_abs_path, new_path, 511);
    new_abs_path[511] = '\0';
  } else {
    fk::memory::copy_string(new_abs_path, cwd);
    if (new_abs_path[0] != '\0' && new_abs_path[fk::memory::length(new_abs_path) - 1] != '/') {
      fk::memory::concatenate(new_abs_path, "/");
    }
    fk::memory::concatenate(new_abs_path, new_path);
  }

  auto res = VirtualFileSystem::the().link(new_abs_path, old_abs_path);
  if (res.is_error())
    return return_error(res.error());

  return 0;
}
}
