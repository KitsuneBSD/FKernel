#include <LibFK/Core/error.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Virtual/SignalFd/signal_fd_node.h>
#include <Kernel/Fs/Vfs/Core/definitions.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

using namespace fkernel;

static uint64_t do_signalfd4(int fd, uint64_t mask_ptr, uint64_t sizemask, int flags) {
    if (sizemask < sizeof(uint64_t))
        return -static_cast<int>(fk::core::Error::InvalidParameter);

    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    uint64_t mask = 0;
    auto copy_res = fkernel::memory::copy_from_user(
        &mask, reinterpret_cast<const void*>(mask_ptr), sizeof(uint64_t));
    if (copy_res.is_error()) return -static_cast<int>(copy_res.error());

    bool nonblock = (flags & O_NONBLOCK) != 0;

    if (fd != -1) {
        auto desc = task->get_file_descriptor(fd);
        if (!desc) return -static_cast<int>(fk::core::Error::InvalidHandle);
        auto node = desc->node();
        if (!node || !node->is_signalfd()) return -static_cast<int>(fk::core::Error::InvalidParameter);
        auto* sfd = static_cast<SignalFdNode*>(node.get());
        sfd->update_mask(mask);
        sfd->set_nonblock(nonblock);
        task->resources.ipc.signals.blocked = mask;
        return static_cast<uint64_t>(fd);
    }

    auto node_res = SignalFdNode::create(task, mask);
    if (node_res.is_error()) return -static_cast<int>(node_res.error());
    node_res.value()->set_nonblock(nonblock);

    task->resources.ipc.signal_fd = node_res.value().get();
    task->resources.ipc.signals.blocked |= mask;

    auto dentry_res = Dentry::create("signalfd", nullptr);
    if (dentry_res.is_error()) return -1;
    auto dentry = dentry_res.value();
    dentry->push_node(node_res.value());

    int oflags = O_RDONLY | (nonblock ? O_NONBLOCK : 0);
    auto desc = fk::make_ref<FileDescription>(dentry, oflags).value();
    int new_fd = task->add_file_descriptor(desc);
    return static_cast<uint64_t>(new_fd);
}

// sys_signalfd4(...) → 0 or -errno
extern "C" uint64_t sys_signalfd4(uint64_t fd, uint64_t mask_ptr, uint64_t sizemask,
                        uint64_t flags, uint64_t, uint64_t,
                        [[maybe_unused]] PtRegs* regs) {
    return do_signalfd4(static_cast<int>(fd), mask_ptr, sizemask, static_cast<int>(flags));
}
