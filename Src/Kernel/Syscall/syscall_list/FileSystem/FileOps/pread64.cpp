#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

// sys_pread64(...) → 0 or -errno
extern "C" uint64_t sys_pread64(uint64_t fd, uint64_t buf_ptr, uint64_t count,
                                 uint64_t offset, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;

    auto desc = task->get_file_descriptor((int)fd);
    if (!desc) return (uint64_t)-9; // EBADF

    auto node = desc->node();
    if (!node) return (uint64_t)-9;

    auto res = node->read(offset, (size_t)count, reinterpret_cast<uint8_t*>(buf_ptr));
    if (res.is_error()) return fkernel::return_error(res.error());
    return (uint64_t)res.value();
}
