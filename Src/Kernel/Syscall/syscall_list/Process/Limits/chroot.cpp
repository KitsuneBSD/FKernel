#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Text/string.h>
#include <LibFK/Utilities/memory.h>

extern "C" uint64_t sys_chroot(uint64_t path_ptr, uint64_t, uint64_t, uint64_t,
                                uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;

    const char* path = reinterpret_cast<const char*>(path_ptr);
    if (!path || !*path) return (uint64_t)-22;

    char resolved[512];
    if (path[0] != '/') {
        const char* cwd = task->resources.files.cwd.c_str();
        size_t cwd_len = fk::memory::length(cwd);
        if (cwd_len + fk::memory::length(path) + 2 >= sizeof(resolved)) return (uint64_t)-22;
        fk::memory::copy_string(resolved, cwd);
        if (cwd_len > 0 && resolved[cwd_len - 1] != '/') fk::memory::concatenate(resolved, "/");
        fk::memory::concatenate(resolved, path);
        path = resolved;
    }

  auto res = fkernel::VirtualFileSystem::the().resolve_path(path);
  if (res.is_error()) return fkernel::return_error(res.error());
  if (!res.value()->top_node() || !res.value()->top_node()->is_directory())
    return (uint64_t)-20; // ENOTDIR

  task->resources.files.root = res.value();
  return 0;
}
