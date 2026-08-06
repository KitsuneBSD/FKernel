#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/file_description.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Posix/sys/stat.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

static constexpr int AT_FDCWD = -100;

extern "C" {

uint64_t sys_newfstatat(uint64_t dirfd, uint64_t path_ptr, uint64_t statbuf_ptr, [[maybe_unused]] uint64_t flags,
                       uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* current_task = SchedulerManager::the().current();
    if (!current_task) return fkernel::return_error(fk::core::Error::PermissionDenied);

    if (!path_ptr || !statbuf_ptr) return fkernel::return_error(fk::core::Error::InvalidParameter);

    if (!fkernel::memory::is_user_address(path_ptr, 1))
        return -14; // EFAULT
    if (!fkernel::memory::is_user_address(statbuf_ptr, sizeof(struct stat)))
        return -14; // EFAULT

    // Copy path from user space
    char kpath[512];
    const char* upath = reinterpret_cast<const char*>(path_ptr);
    size_t plen = 0;
    while (plen < sizeof(kpath) - 1 && upath[plen] != '\0')
        plen++;
    if (plen >= sizeof(kpath) - 1) return -14; // EFAULT
    auto cp = fkernel::memory::copy_from_user(kpath, upath, plen + 1);
    if (cp.is_error()) return -14; // EFAULT

    const char* path = kpath;

    char absolute_path[512];
    if (path[0] == '/') {
        fk::memory::copy_n(absolute_path, path, 511);
        absolute_path[511] = '\0';
    } else if (static_cast<int>(dirfd) == AT_FDCWD) {
        snprintf(absolute_path, 512, "%s/%s", current_task->resources.files.cwd.c_str(), path);
    } else {
        auto desc = current_task->get_file_descriptor(static_cast<int>(dirfd));
        if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
        auto dir_dentry = desc->dentry();
        if (!dir_dentry) return fkernel::return_error(fk::core::Error::InvalidHandle);
        auto dir_path = dir_dentry->get_path();
        snprintf(absolute_path, 512, "%s/%s", dir_path.c_str(), path);
    }

    struct stat kstat;
    auto res = VirtualFileSystem::the().stat(absolute_path, &kstat);
    if (res.is_error()) return fkernel::return_error(res.error());

    auto wres = fkernel::memory::copy_to_user(reinterpret_cast<void*>(statbuf_ptr), &kstat, sizeof(kstat));
    if (wres.is_error()) return -14; // EFAULT

    return 0;
}

}