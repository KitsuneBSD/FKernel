#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

[[maybe_unused]] static constexpr int AT_FDCWD = -100;

// Resolve a path relative to dirfd. Writes result into `out` (at least 512 bytes).
// Returns true on success.
static bool resolve_at(Task* task, int dirfd, const char* path, char* out, size_t out_size) {
    if (!path || !out || out_size < 2) return false;
    if (path[0] == '/') {
        fk::memory::copy_n(out, path, out_size - 1);
        out[out_size - 1] = '\0';
        return true;
    }
    // For AT_FDCWD or any real dirfd (best-effort: use task cwd for now)
    (void)dirfd;
    const char* cwd = task->resources.files.cwd.c_str();
    size_t cwd_len = fk::memory::length(cwd);
    if (cwd_len + fk::memory::length(path) + 2 >= out_size) return false;
    fk::memory::copy_string(out, cwd);
    if (cwd_len > 0 && out[cwd_len - 1] != '/') { out[cwd_len++] = '/'; out[cwd_len] = '\0'; }
    fk::memory::concatenate(out, path);
    return true;
}

// sys_mkdirat(...) → 0 or -errno
extern "C" uint64_t sys_mkdirat(uint64_t dirfd_u, uint64_t path_ptr, uint64_t mode,
                     uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    const char* path = reinterpret_cast<const char*>(path_ptr);
    if (!path) return (uint64_t)-22;

    char resolved[512];
    if (!resolve_at(task, (int)dirfd_u, path, resolved, sizeof(resolved))) return (uint64_t)-22;

    auto res = VirtualFileSystem::the().mkdir(resolved, (int)mode);
    if (res.is_error()) return return_error(res.error());
    return 0;
}
