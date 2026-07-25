#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Memory/UserAccess/user_access.h>

struct statfs_buf {
    long f_type;
    long f_bsize;
    long f_blocks;
    long f_bfree;
    long f_bavail;
    long f_files;
    long f_ffree;
    struct { int32_t val[2]; } f_fsid;
    long f_namelen;
    long f_frsize;
    long f_flags;
    long f_spare[4];
};

static void fill_statfs(statfs_buf* s) {
    auto& pmm = PhysicalMemoryManager::the();
    size_t total = pmm.total_memory();
    size_t free_mem = pmm.free_memory();

    constexpr long BLOCK_SIZE = 4096;
    s->f_type    = 0xEF53;  // EXT2_SUPER_MAGIC — generic placeholder
    s->f_bsize   = BLOCK_SIZE;
    s->f_blocks  = (long)(total / BLOCK_SIZE);
    s->f_bfree   = (long)(free_mem / BLOCK_SIZE);
    s->f_bavail  = (long)(free_mem / BLOCK_SIZE);
    s->f_files   = (long)(total / BLOCK_SIZE / 4);  // rough inode estimate
    s->f_ffree   = (long)(free_mem / BLOCK_SIZE / 4);
    s->f_fsid.val[0] = 0;
    s->f_fsid.val[1] = 0;
    s->f_namelen = 255;
    s->f_frsize  = BLOCK_SIZE;
    s->f_flags   = 0;
    for (int i = 0; i < 4; ++i) s->f_spare[i] = 0;
}

extern "C" {
uint64_t sys_statfs(uint64_t path_ptr, uint64_t buf_ptr,
                    uint64_t, uint64_t, uint64_t, uint64_t,
                    [[maybe_unused]] PtRegs* regs) {
    if (!path_ptr || !buf_ptr) return (uint64_t)-22; // EINVAL
    if (!fkernel::memory::is_user_address(static_cast<uintptr_t>(buf_ptr), sizeof(statfs_buf)))
        return (uint64_t)-14; // EFAULT

    statfs_buf s{};
    fill_statfs(&s);
    auto res = fkernel::memory::copy_to_user(reinterpret_cast<void*>(buf_ptr), &s, sizeof(s));
    return res.is_error() ? (uint64_t)-14 : 0;
}

uint64_t sys_fstatfs(uint64_t fd, uint64_t buf_ptr,
                     uint64_t, uint64_t, uint64_t, uint64_t,
                     [[maybe_unused]] PtRegs* regs) {
    if (!buf_ptr) return (uint64_t)-22;
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;
    auto desc = task->get_file_descriptor((int)fd);
    if (!desc) return fkernel::return_error(fk::core::Error::InvalidHandle);
    if (!fkernel::memory::is_user_address(static_cast<uintptr_t>(buf_ptr), sizeof(statfs_buf)))
        return (uint64_t)-14;

    statfs_buf s{};
    fill_statfs(&s);
    auto res = fkernel::memory::copy_to_user(reinterpret_cast<void*>(buf_ptr), &s, sizeof(s));
    return res.is_error() ? (uint64_t)-14 : 0;
}
}
