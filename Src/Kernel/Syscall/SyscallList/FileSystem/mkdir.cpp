#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>

using namespace fkernel;

extern "C" {
uint64_t sys_mkdir(uint64_t path_ptr, uint64_t mode, uint64_t, uint64_t,
                   uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* current_task = SchedulerManager::the().current();
    if (!current_task) return -1;

    const char* path = reinterpret_cast<const char*>(path_ptr);
    if (!path) return -static_cast<int>(fk::core::Error::InvalidParameter);

    char absolute_path[512];
    if (path[0] != '/') {
        strcpy(absolute_path, current_task->resources.files.cwd.c_str());
        if (absolute_path[strlen(absolute_path)-1] != '/') {
            strcat(absolute_path, "/");
        }
        strcat(absolute_path, path);
        path = absolute_path;
    }

    auto res = VirtualFileSystem::the().mkdir(path, (int)mode);
    if (res.is_error()) return -static_cast<int>(res.error());

    return 0;
}
}