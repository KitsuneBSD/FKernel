#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

extern "C" {

uint64_t sys_utimes(uint64_t path_ptr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                    [[maybe_unused]] PtRegs* regs) {
  const char* path = reinterpret_cast<const char*>(path_ptr);
  if (!path)
    return return_error(fk::core::Error::InvalidParameter);
  auto res = VirtualFileSystem::the().resolve_path(path);
  if (res.is_error())
    return return_error(res.error());
  return 0;
}

uint64_t sys_futimesat(uint64_t dirfd_u, uint64_t path_ptr, uint64_t, uint64_t, uint64_t,
                       uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task)
    return (uint64_t)-1;
  const char* path = reinterpret_cast<const char*>(path_ptr);
  if (!path)
    return return_error(fk::core::Error::InvalidParameter);
  char resolved[512];
  static constexpr int AT_FDCWD = -100;
  if (path[0] == '/') {
    fk::memory::copy_n(resolved, path, sizeof(resolved) - 1);
    resolved[sizeof(resolved) - 1] = '\0';
  } else if (static_cast<int>(dirfd_u) == AT_FDCWD) {
    const char* cwd = task->resources.files.cwd.c_str();
    size_t cwd_len = fk::memory::length(cwd);
    fk::memory::copy_string(resolved, cwd);
    if (cwd_len > 0 && resolved[cwd_len - 1] != '/') { resolved[cwd_len++] = '/'; resolved[cwd_len] = '\0'; }
    fk::memory::concatenate(resolved, path);
  } else {
    return return_error(fk::core::Error::NotImplemented);
  }
  auto res = VirtualFileSystem::the().resolve_path(resolved);
  if (res.is_error())
    return return_error(res.error());
  return 0;
}

uint64_t sys_utimensat(uint64_t dirfd_u, uint64_t path_ptr, uint64_t, uint64_t flags_u,
                       uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task)
    return (uint64_t)-1;
  const char* path = reinterpret_cast<const char*>(path_ptr);
  if (!path)
    return return_error(fk::core::Error::InvalidParameter);
  (void)flags_u;
  char resolved[512];
  static constexpr int AT_FDCWD = -100;
  if (path[0] == '/') {
    fk::memory::copy_n(resolved, path, sizeof(resolved) - 1);
    resolved[sizeof(resolved) - 1] = '\0';
  } else if (static_cast<int>(dirfd_u) == AT_FDCWD) {
    const char* cwd = task->resources.files.cwd.c_str();
    size_t cwd_len = fk::memory::length(cwd);
    fk::memory::copy_string(resolved, cwd);
    if (cwd_len > 0 && resolved[cwd_len - 1] != '/') { resolved[cwd_len++] = '/'; resolved[cwd_len] = '\0'; }
    fk::memory::concatenate(resolved, path);
  } else {
    return return_error(fk::core::Error::NotImplemented);
  }
  auto res = VirtualFileSystem::the().resolve_path(resolved);
  if (res.is_error())
    return return_error(res.error());
  return 0;
}

}
