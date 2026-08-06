#include <LibFK/Core/error.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Virtual/TimerFd/timer_fd_node.h>
#include <Kernel/Fs/Vfs/Core/definitions.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

using namespace fkernel;

// sys_timerfd_settime(...) → 0 or -errno
extern "C" uint64_t sys_timerfd_settime(uint64_t timerfd, uint64_t flags, uint64_t new_value_ptr,
                              uint64_t old_value_ptr, uint64_t, uint64_t,
                              [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto desc = task->get_file_descriptor(static_cast<int>(timerfd));
    if (!desc) return -static_cast<int>(fk::core::Error::InvalidHandle);

    auto node = desc->node();
    if (!node || !node->is_timerfd()) return -static_cast<int>(fk::core::Error::InvalidParameter);

    TimerFdNode* tfd = static_cast<TimerFdNode*>(node.get());

    KernelItimerspec new_spec{};
    auto copy_res = fkernel::memory::copy_from_user(
        &new_spec, reinterpret_cast<const void*>(new_value_ptr), sizeof(new_spec));
    if (copy_res.is_error()) return -static_cast<int>(copy_res.error());

    KernelItimerspec old_spec{};
    KernelItimerspec* old_ptr = old_value_ptr ? &old_spec : nullptr;
    tfd->settime(new_spec, old_ptr);

    if (old_value_ptr && old_ptr) {
        auto copy_out = fkernel::memory::copy_to_user(
            reinterpret_cast<void*>(old_value_ptr), &old_spec, sizeof(old_spec));
        if (copy_out.is_error()) return -static_cast<int>(copy_out.error());
    }

    (void)flags;
    return 0;
}
