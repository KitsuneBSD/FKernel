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

// sys_timerfd_gettime(...) → 0 or -errno
extern "C" uint64_t sys_timerfd_gettime(uint64_t timerfd, uint64_t curr_value_ptr,
                              uint64_t, uint64_t, uint64_t, uint64_t,
                              [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto desc = task->get_file_descriptor(static_cast<int>(timerfd));
    if (!desc) return -static_cast<int>(fk::core::Error::InvalidHandle);

    auto node = desc->node();
    if (!node || !node->is_timerfd()) return -static_cast<int>(fk::core::Error::InvalidParameter);

    TimerFdNode* tfd = static_cast<TimerFdNode*>(node.get());

    KernelItimerspec curr{};
    tfd->gettime(curr);

    auto copy_res = fkernel::memory::copy_to_user(
        reinterpret_cast<void*>(curr_value_ptr), &curr, sizeof(curr));
    if (copy_res.is_error()) return -static_cast<int>(copy_res.error());

    return 0;
}
