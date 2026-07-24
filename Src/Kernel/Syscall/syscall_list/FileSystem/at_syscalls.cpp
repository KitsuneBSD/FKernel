#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

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

extern "C" {

uint64_t sys_openat(uint64_t dirfd_u, uint64_t path_ptr, uint64_t flags,
                    [[maybe_unused]] uint64_t mode, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    const char* path = reinterpret_cast<const char*>(path_ptr);
    if (!path) return (uint64_t)-22;

    char resolved[512];
    if (!resolve_at(task, (int)dirfd_u, path, resolved, sizeof(resolved))) return (uint64_t)-22;

    auto res = VirtualFileSystem::the().open(resolved, (int)flags);
    if (res.is_error()) return return_error(res.error());

    int fd = task->add_file_descriptor(res.value());
    if (fd < 0) return (uint64_t)fd;

    if (flags & 02000000) res.value()->set_cloexec(true); // O_CLOEXEC
    return (uint64_t)fd;
}

uint64_t sys_mkdirat(uint64_t dirfd_u, uint64_t path_ptr, uint64_t mode,
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

uint64_t sys_unlinkat(uint64_t dirfd_u, uint64_t path_ptr, uint64_t flags,
                      uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    const char* path = reinterpret_cast<const char*>(path_ptr);
    if (!path) return (uint64_t)-22;

    char resolved[512];
    if (!resolve_at(task, (int)dirfd_u, path, resolved, sizeof(resolved))) return (uint64_t)-22;

    static constexpr int AT_REMOVEDIR = 0x200;
    if (flags & AT_REMOVEDIR) {
        auto res = VirtualFileSystem::the().rmdir(resolved);
        if (res.is_error()) return return_error(res.error());
    } else {
        auto res = VirtualFileSystem::the().unlink(resolved);
        if (res.is_error()) return return_error(res.error());
    }
    return 0;
}

uint64_t sys_renameat(uint64_t old_dirfd_u, uint64_t old_path_ptr,
                      uint64_t new_dirfd_u, uint64_t new_path_ptr,
                      uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    const char* old_path = reinterpret_cast<const char*>(old_path_ptr);
    const char* new_path = reinterpret_cast<const char*>(new_path_ptr);
    if (!old_path || !new_path) return (uint64_t)-22;

    char old_resolved[512], new_resolved[512];
    if (!resolve_at(task, (int)old_dirfd_u, old_path, old_resolved, sizeof(old_resolved))) return (uint64_t)-22;
    if (!resolve_at(task, (int)new_dirfd_u, new_path, new_resolved, sizeof(new_resolved))) return (uint64_t)-22;

    auto res = VirtualFileSystem::the().rename(old_resolved, new_resolved);
    if (res.is_error()) return return_error(res.error());
    return 0;
}

uint64_t sys_symlinkat(uint64_t target_ptr, uint64_t new_dirfd_u, uint64_t path_ptr,
                       uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    const char* target = reinterpret_cast<const char*>(target_ptr);
    const char* path = reinterpret_cast<const char*>(path_ptr);
    if (!target || !path) return (uint64_t)-22;

    char resolved[512];
    if (!resolve_at(task, (int)new_dirfd_u, path, resolved, sizeof(resolved))) return (uint64_t)-22;

    auto res = VirtualFileSystem::the().symlink(resolved, target);
    if (res.is_error()) return return_error(res.error());
    return 0;
}

uint64_t sys_linkat(uint64_t old_dirfd_u, uint64_t old_path_ptr,
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

uint64_t sys_readlinkat(uint64_t dirfd_u, uint64_t path_ptr, uint64_t buf_ptr,
                        uint64_t bufsiz, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    const char* path = reinterpret_cast<const char*>(path_ptr);
    char* buf = reinterpret_cast<char*>(buf_ptr);
    if (!path || !buf) return (uint64_t)-22;

    char resolved[512];
    if (!resolve_at(task, (int)dirfd_u, path, resolved, sizeof(resolved))) return (uint64_t)-22;

    auto dentry_res = VirtualFileSystem::the().resolve_path(resolved);
    if (dentry_res.is_error()) return return_error(fk::core::Error::NotFound);

    auto node = dentry_res.value()->top_node();
    if (!node || !node->is_symlink()) return return_error(fk::core::Error::NotASymlink);

    auto link_res = node->read_link();
    if (link_res.is_error()) return return_error(link_res.error());

    const auto& target = link_res.value();
    size_t copy_len = target.length() < bufsiz ? target.length() : bufsiz;
    for (size_t i = 0; i < copy_len; ++i) buf[i] = target[i];
    return (uint64_t)copy_len;
}

uint64_t sys_fchmodat(uint64_t dirfd_u, uint64_t path_ptr, uint64_t mode,
                      uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    const char* path = reinterpret_cast<const char*>(path_ptr);
    if (!path) return (uint64_t)-22;
    char resolved[512];
    if (!resolve_at(task, (int)dirfd_u, path, resolved, sizeof(resolved))) return (uint64_t)-22;
    auto res = VirtualFileSystem::the().resolve_path(resolved);
    if (res.is_error()) return return_error(res.error());
    auto node = res.value()->top_node();
    if (!node) return (uint64_t)-2;
    node->set_permissions((uint32_t)mode);
    return 0;
}

uint64_t sys_fchownat(uint64_t dirfd_u, uint64_t path_ptr, uint64_t uid,
                      uint64_t gid, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    const char* path = reinterpret_cast<const char*>(path_ptr);
    if (!path) return (uint64_t)-22;
    char resolved[512];
    if (!resolve_at(task, (int)dirfd_u, path, resolved, sizeof(resolved))) return (uint64_t)-22;
    auto res = VirtualFileSystem::the().resolve_path(resolved);
    if (res.is_error()) return return_error(res.error());
    auto node = res.value()->top_node();
    if (!node) return (uint64_t)-2;
    // -1 (0xFFFFFFFF) means "don't change"
    uint32_t new_uid = (uid == (uint64_t)-1) ? node->node_uid() : (uint32_t)uid;
    uint32_t new_gid = (gid == (uint64_t)-1) ? node->node_gid() : (uint32_t)gid;
    node->set_owner(new_uid, new_gid);
    return 0;
}

uint64_t sys_faccessat(uint64_t dirfd_u, uint64_t path_ptr, uint64_t mode,
                       uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    const char* path = reinterpret_cast<const char*>(path_ptr);
    if (!path) return (uint64_t)-22;

    char resolved[512];
    if (!resolve_at(task, (int)dirfd_u, path, resolved, sizeof(resolved))) return (uint64_t)-22;

    auto res = VirtualFileSystem::the().resolve_path(resolved);
    if (res.is_error()) return return_error(res.error());
    auto node = res.value()->top_node();
    if (!node) return (uint64_t)-2;

    uint32_t file_mode = node->node_mode();
    uint32_t file_uid = node->node_uid();
    uint32_t file_gid = node->node_gid();
    uint32_t euid = task->control.identity.euid;
    uint32_t egid = task->control.identity.egid;

    // F_OK = 0: file exists (already verified by resolve_path)
    if ((int)mode == 0) return 0;

    // Determine which permission bits to check
    uint32_t perms;
    if (euid == 0) {
        // Root: only check exec bit if X_OK requested
        perms = (mode & 1) ? (file_mode >> 6) : file_mode;
    } else if (euid == file_uid) {
        perms = (file_mode >> 6) & 7;  // owner bits
    } else if (egid == file_gid || task->control.identity.ngroups > 0) {
        perms = (file_mode >> 3) & 7;  // group bits
    } else {
        perms = file_mode & 7;          // other bits
    }

    // Check requested access
    if ((mode & 4) && !(perms & 4)) return (uint64_t)-13; // EACCES (R_OK)
    if ((mode & 2) && !(perms & 2)) return (uint64_t)-13; // EACCES (W_OK)
    if ((mode & 1) && !(perms & 1)) return (uint64_t)-13; // EACCES (X_OK)
    return 0;
}

uint64_t sys_mknodat(uint64_t dirfd_u, uint64_t path_ptr, uint64_t mode,
                     uint64_t dev, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    (void)dev; (void)mode;
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    const char* path = reinterpret_cast<const char*>(path_ptr);
    if (!path) return (uint64_t)-22;

    char resolved[512];
    if (!resolve_at(task, (int)dirfd_u, path, resolved, sizeof(resolved))) return (uint64_t)-22;

    // Create a regular file (device files not supported yet)
    auto res = VirtualFileSystem::the().open(resolved, 0x241); // O_CREAT|O_WRONLY|O_TRUNC
    if (res.is_error()) return return_error(res.error());
    return 0;
}

} // extern "C"
