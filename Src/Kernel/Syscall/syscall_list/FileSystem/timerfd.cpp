#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/TimerFd/timer_fd_node.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/error.h>
#include <LibFK/Memory/ref_ptr.h>

using namespace fkernel;

extern "C" {

uint64_t sys_timerfd_create(uint64_t clockid, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                             [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto node_res = TimerFdNode::create(static_cast<int>(clockid));
    if (node_res.is_error()) return -static_cast<int>(node_res.error());

    auto dentry_res = Dentry::create("timerfd", nullptr);
    if (dentry_res.is_error()) return -1;
    auto dentry = dentry_res.value();
    dentry->push_node(node_res.value());

    auto desc = fk::make_ref<FileDescription>(dentry, O_RDONLY).value();
    int fd = task->add_file_descriptor(desc);
    return static_cast<uint64_t>(fd);
}

uint64_t sys_timerfd_settime(uint64_t timerfd, uint64_t flags, uint64_t new_value_ptr,
                              uint64_t old_value_ptr, uint64_t, uint64_t,
                              [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto desc = task->get_file_descriptor(static_cast<int>(timerfd));
    if (!desc) return -static_cast<int>(fk::core::Error::InvalidHandle);

    auto node = desc->node();
    if (!node || !node->is_timerfd()) return -static_cast<int>(fk::core::Error::InvalidParameter);

    TimerFdNode* tfd = static_cast<TimerFdNode*>(node.get());

    TimerFdNode::KernelItimerspec new_spec{};
    auto copy_res = fkernel::memory::copy_from_user(
        &new_spec, reinterpret_cast<const void*>(new_value_ptr), sizeof(new_spec));
    if (copy_res.is_error()) return -static_cast<int>(copy_res.error());

    TimerFdNode::KernelItimerspec old_spec{};
    TimerFdNode::KernelItimerspec* old_ptr = old_value_ptr ? &old_spec : nullptr;
    tfd->settime(new_spec, old_ptr);

    if (old_value_ptr && old_ptr) {
        auto copy_out = fkernel::memory::copy_to_user(
            reinterpret_cast<void*>(old_value_ptr), &old_spec, sizeof(old_spec));
        if (copy_out.is_error()) return -static_cast<int>(copy_out.error());
    }

    (void)flags;
    return 0;
}

uint64_t sys_timerfd_gettime(uint64_t timerfd, uint64_t curr_value_ptr,
                              uint64_t, uint64_t, uint64_t, uint64_t,
                              [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto desc = task->get_file_descriptor(static_cast<int>(timerfd));
    if (!desc) return -static_cast<int>(fk::core::Error::InvalidHandle);

    auto node = desc->node();
    if (!node || !node->is_timerfd()) return -static_cast<int>(fk::core::Error::InvalidParameter);

    TimerFdNode* tfd = static_cast<TimerFdNode*>(node.get());

    TimerFdNode::KernelItimerspec curr{};
    tfd->gettime(curr);

    auto copy_res = fkernel::memory::copy_to_user(
        reinterpret_cast<void*>(curr_value_ptr), &curr, sizeof(curr));
    if (copy_res.is_error()) return -static_cast<int>(copy_res.error());

    return 0;
}

}
