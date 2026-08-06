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

// sys_linkat(...) → 0 or -errno
extern "C" uint64_t sys_linkat(uint64_t old_dirfd_u, uint64_t old_path_ptr,
                    uint64_t new_dirfd_u, uint64_t new_path_ptr,
                    uint64_t flags, uint64_t, [[maybe_unused]] PtRegs* regs) {
    (void)flags;
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    const char* old_path = reinterpret_cast<const char*>(old_path_ptr);
    const char* new_path = reinterpret_cast<const char*>(new_path_ptr);
    if (!old_path || !new_path) return (uint64_t)-22;

    char old_resolved[512], new_resolved[512];
    if (!resolve_at(task, (int)old_dirfd_u, old_path, old_resolved, sizeof(old_resolved))) return (uint64_t)-22;
    if (!resolve_at(task, (int)new_dirfd_u, new_path, new_resolved, sizeof(new_resolved))) return (uint64_t)-22;

    auto res = VirtualFileSystem::the().link(old_resolved, new_resolved);
    if (res.is_error()) return return_error(res.error());
    return 0;
}
