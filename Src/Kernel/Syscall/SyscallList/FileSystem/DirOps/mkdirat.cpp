#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

[[maybe_unused]] static constexpr int AT_FDCWD = -100;

// sys_mkdirat(...) → 0 or -errno
extern "C" uint64_t sys_mkdirat(uint64_t dirfd_u, uint64_t path_ptr, uint64_t mode,
                     uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    if (!path_ptr) return (uint64_t)-22;

    char kpath[512];
    auto copy_res = fkernel::memory::copy_string_from_user(kpath, reinterpret_cast<const void*>(path_ptr), sizeof(kpath));
    if (copy_res.is_error()) return (uint64_t)-14;

    char resolved[512];
    if (kpath[0] == '/') {
        fk::memory::copy_n(resolved, kpath, sizeof(resolved) - 1);
        resolved[sizeof(resolved) - 1] = '\0';
    } else {
        // For AT_FDCWD or any real dirfd (best-effort: use task cwd)
        (void)dirfd_u;
        const char* cwd = task->resources.files.cwd.c_str();
        size_t cwd_len = fk::memory::length(cwd);
        if (cwd_len + fk::memory::length(kpath) + 2 >= sizeof(resolved))
            return (uint64_t)-22;
        fk::memory::copy_string(resolved, cwd);
        if (cwd_len > 0 && resolved[cwd_len - 1] != '/') {
            resolved[cwd_len++] = '/';
            resolved[cwd_len] = '\0';
        }
        fk::memory::concatenate(resolved, kpath);
    }

    auto res = VirtualFileSystem::the().mkdir(resolved, (int)mode);
    if (res.is_error()) return return_error(res.error());
    return 0;
}
