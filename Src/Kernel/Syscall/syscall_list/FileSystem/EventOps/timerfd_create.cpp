#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Virtual/TimerFd/timer_fd_node.h>
#include <Kernel/Fs/Vfs/Core/definitions.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/error.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>

using namespace fkernel;

// sys_timerfd_create(...) → 0 or -errno
extern "C" uint64_t sys_timerfd_create(uint64_t clockid, uint64_t flags, uint64_t, uint64_t, uint64_t, uint64_t,
                             [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto node_res = TimerFdNode::create(static_cast<int>(clockid));
    if (node_res.is_error()) return -static_cast<int>(node_res.error());

    bool nonblock = (flags & O_NONBLOCK) != 0;

    node_res.value()->set_nonblock(nonblock);

    auto dentry_res = Dentry::create("timerfd", nullptr);
    if (dentry_res.is_error()) return -1;
    auto dentry = dentry_res.value();
    dentry->push_node(node_res.value());

    int oflags = O_RDONLY | (nonblock ? O_NONBLOCK : 0);
    auto desc = fk::make_ref<FileDescription>(dentry, oflags).value();
    int fd = task->add_file_descriptor(desc);
    return static_cast<uint64_t>(fd);
}
