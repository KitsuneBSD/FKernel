#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

extern "C" {
uint64_t sys_creat(uint64_t path_ptr, [[maybe_unused]] uint64_t mode, uint64_t, uint64_t, uint64_t,
                   uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return -1;

  const char* path = reinterpret_cast<const char*>(path_ptr);
  if (!path)
    return -static_cast<int>(fk::core::Error::InvalidParameter);

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

  auto res = VirtualFileSystem::the().open(path, O_CREAT | O_WRONLY | O_TRUNC);
  if (res.is_error())
    return return_error(res.error());

  int fd = current_task->add_file_descriptor(res.value());
  if (fd < 0)
    return (uint64_t)fd;

  return (uint64_t)fd;
}
}
