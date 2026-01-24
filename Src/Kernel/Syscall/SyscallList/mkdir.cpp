#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/Error.h>
#include <LibC/string.h>

extern "C" {
uint64_t sys_mkdir(uint64_t path_ptr, uint64_t mode, uint64_t, uint64_t,
                   uint64_t, uint64_t) {
    auto* current_task = SchedulerManager::the().current();
    if (!current_task) return -1;

    const char* path = reinterpret_cast<const char*>(path_ptr);
    if (!path) return -static_cast<int>(fk::core::Error::InvalidParameter);

    char absolute_path[512];
    if (path[0] != '/') {
        strcpy(absolute_path, current_task->cwd.c_str());
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
