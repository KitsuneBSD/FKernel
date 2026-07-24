#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel;

static bool check_permission(uint32_t mode, uint32_t file_uid, uint32_t file_gid,
                              uint32_t caller_uid, uint32_t caller_gid,
                              const uint32_t* supp_gids, uint32_t ngroups,
                              int access_mode) {
    // root: R/W always granted; X only if any x bit is set
    if (caller_uid == 0) {
        if (access_mode & X_OK)
            return (mode & 0111) != 0;
        return true;
    }

    if (caller_uid == file_uid) {
        uint32_t perm_bits = (mode >> 6) & 7;
        if ((access_mode & R_OK) && !(perm_bits & 4)) return false;
        if ((access_mode & W_OK) && !(perm_bits & 2)) return false;
        if ((access_mode & X_OK) && !(perm_bits & 1)) return false;
        return true;
    }

    bool in_group = (caller_gid == file_gid);
    if (!in_group) {
        for (uint32_t i = 0; i < ngroups && i < 16; ++i) {
            if (supp_gids[i] == file_gid) { in_group = true; break; }
        }
    }
    uint32_t perm_bits = in_group ? ((mode >> 3) & 7) : (mode & 7);

    if ((access_mode & R_OK) && !(perm_bits & 4)) return false;
    if ((access_mode & W_OK) && !(perm_bits & 2)) return false;
    if ((access_mode & X_OK) && !(perm_bits & 1)) return false;
    return true;
}

extern "C" {
uint64_t sys_access(uint64_t path_ptr, uint64_t mode, uint64_t, uint64_t, uint64_t,
                    uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;

    const char* path = reinterpret_cast<const char*>(path_ptr);
    if (!path) return return_error(fk::core::Error::InvalidParameter);

    char absolute_path[512];
    if (path[0] != '/') {
        const char* cwd = task->resources.files.cwd.c_str();
        size_t cwd_len = fk::memory::length(cwd);
        if (cwd_len + fk::memory::length(path) + 2 >= 512)
            return return_error(fk::core::Error::InvalidParameter);
        fk::memory::copy_string(absolute_path, cwd);
        if (cwd_len > 0 && absolute_path[cwd_len - 1] != '/')
            fk::memory::concatenate(absolute_path, "/");
        fk::memory::concatenate(absolute_path, path);
        path = absolute_path;
    }

    auto dentry_res = VirtualFileSystem::the().resolve_path(path);
    if (dentry_res.is_error())
        return return_error(fk::core::Error::NotFound);

    auto node = dentry_res.value()->top_node();
    if (!node) return return_error(fk::core::Error::NotFound);

    int access_mode = (int)(mode & 7);
    if (access_mode == F_OK) return 0; // file exists — already confirmed

    uint32_t file_mode = node->node_mode();
    uint32_t file_uid  = node->node_uid();
    uint32_t file_gid  = node->node_gid();

    // access() uses real uid/gid per POSIX
    uint32_t caller_uid = task->control.identity.uid;
    uint32_t caller_gid = task->control.identity.gid;

    if (!check_permission(file_mode, file_uid, file_gid,
                          caller_uid, caller_gid,
                          task->control.identity.supplementary_gids,
                          task->control.identity.ngroups,
                          access_mode))
        return return_error(fk::core::Error::PermissionDenied);

    return 0;
}
}
