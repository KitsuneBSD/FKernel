#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibC/string.h>

using namespace fkernel;

extern "C" {

uint64_t sys_newfstatat(uint64_t dirfd, uint64_t path_ptr, uint64_t statbuf_ptr, [[maybe_unused]] uint64_t flags,
                       uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* current_task = SchedulerManager::the().current();
    if (!current_task) return fkernel::return_error(fk::core::Error::PermissionDenied);

    const char* path = reinterpret_cast<const char*>(path_ptr);
    auto* buf = reinterpret_cast<struct stat*>(statbuf_ptr);

    if (!path || !buf) return fkernel::return_error(fk::core::Error::InvalidParameter);

    char absolute_path[512];
    if (path[0] == '/') {
        strncpy(absolute_path, path, 511);
    } else {
        // Handle dirfd (simplified: if AT_FDCWD, use current cwd)
        // AT_FDCWD is -100 in Linux
        if (static_cast<int>(dirfd) == -100) {
            snprintf(absolute_path, 512, "%s/%s", current_task->files.cwd.c_str(), path);
        } else {
            // Real implementation would look up dirfd
            snprintf(absolute_path, 512, "/%s", path); 
        }
    }

    fk::algorithms::klog("SYSCALL", "sys_newfstatat: dirfd=%ld path=%s", (long)dirfd, absolute_path);

    auto res = VirtualFileSystem::the().stat(absolute_path, buf);
    if (res.is_error()) return fkernel::return_error(res.error());

    return 0;
}

}
