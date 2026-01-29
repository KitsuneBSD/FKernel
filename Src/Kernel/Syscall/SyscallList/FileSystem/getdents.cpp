#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>

#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibC/stddef.h>

extern "C" {

uint64_t sys_getdents(uint64_t fd, uint64_t buffer_ptr, uint64_t max_bytes, uint64_t,
                    uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* current_task = SchedulerManager::the().current();
    if (!current_task) return fkernel::return_error(fk::core::Error::PermissionDenied);

    auto description = current_task->get_file_descriptor(static_cast<int>(fd));
    if (!description) return fkernel::return_error(fk::core::Error::InvalidParameter);

    fk::algorithms::klog("SYSCALL", "sys_getdents (32-bit): fd=%lu, buffer=%p, size=%zu, offset=%lu", 
                         fd, (void*)buffer_ptr, (size_t)max_bytes, description->offset());

    fk::containers::Vector<DirectoryEntry> entries;
    
    // Manual readdir logic for 32-bit dirent
    DirectoryEntry dot;
    strcpy(dot.name, ".");
    dot.type = 1;
    entries.push_back(dot);
    DirectoryEntry dotdot;
    strcpy(dotdot.name, "..");
    dotdot.type = 1;
    entries.push_back(dotdot);

    auto list_res = description->node()->list_dir(entries);
    if (list_res.is_error()) return fkernel::return_error(list_res.error());

    uint8_t* buffer = reinterpret_cast<uint8_t*>(buffer_ptr);
    uint64_t bytes_written = 0;
    uint64_t start_idx = description->offset();

    for (size_t i = start_idx; i < entries.size(); ++i) {
        auto& entry = entries[i];
        size_t name_len = strlen(entry.name);
        // linux_dirent: ino(4) + off(4) + reclen(2) + name(n) + pad
        size_t reclen = (offsetof(linux_dirent, d_name) + name_len + 1 + 3) & ~3;

        if (bytes_written + reclen > max_bytes) break;

        auto* dirent = reinterpret_cast<linux_dirent*>(buffer + bytes_written);
        dirent->d_ino = i + 1;
        dirent->d_off = i + 1;
        dirent->d_reclen = static_cast<uint16_t>(reclen);
        
        memcpy(dirent->d_name, entry.name, name_len);
        dirent->d_name[name_len] = '\0';

        bytes_written += reclen;
        description->set_offset(i + 1);
    }

    fk::algorithms::klog("SYSCALL", "sys_getdents: returned %zu bytes, next offset=%lu", bytes_written, description->offset());
    return bytes_written;
}

}
