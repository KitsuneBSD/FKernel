#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

static bool copy_user_string(const char* user_ptr, char* kbuf, size_t kbuf_size) {
  if (!user_ptr || !fkernel::memory::is_user_address(reinterpret_cast<uintptr_t>(user_ptr), 1))
    return false;
  size_t len = 0;
  while (len < kbuf_size - 1 && user_ptr[len] != '\0')
    len++;
  if (len >= kbuf_size - 1) return false;
  auto res = fkernel::memory::copy_from_user(kbuf, user_ptr, len + 1);
  return res.is_ok();
}

extern "C" {
uint64_t sys_pivot_root(uint64_t new_root_ptr, uint64_t put_old_ptr, uint64_t, uint64_t,
                        uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return fkernel::return_error(fk::core::Error::PermissionDenied);
  if (task->control.identity.euid != 0) return -1; // EPERM

  if (!new_root_ptr || !put_old_ptr) return fkernel::return_error(fk::core::Error::InvalidParameter);

  char knew_root[512] = {};
  char kput_old[512] = {};

  if (!copy_user_string(reinterpret_cast<const char*>(new_root_ptr), knew_root, sizeof(knew_root)))
    return -14;
  if (!copy_user_string(reinterpret_cast<const char*>(put_old_ptr), kput_old, sizeof(kput_old)))
    return -14;

  auto check = fkernel::VirtualFileSystem::the().resolve_path(kput_old);
  if (check.is_error()) {
    auto mkres = fkernel::VirtualFileSystem::the().mkdir(kput_old, 0755);
    if (mkres.is_error()) return fkernel::return_error(mkres.error());
  }

  auto res = fkernel::VirtualFileSystem::the().pivot_root(knew_root, kput_old);
  if (res.is_error()) return fkernel::return_error(res.error());

  task->resources.files.root = nullptr;
  if (task->resources.files.cwd.c_str()[0] == '/') {
    auto cwd_dentry = fkernel::VirtualFileSystem::the().resolve_path(task->resources.files.cwd.c_str());
    if (cwd_dentry.is_error()) {
      task->resources.files.cwd = fk::text::fixed_string<256>("/");
    }
  }

  return 0;
}
} // extern "C"
