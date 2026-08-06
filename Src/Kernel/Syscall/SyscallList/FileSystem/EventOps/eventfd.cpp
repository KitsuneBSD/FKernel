#include <LibFK/Core/error.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Virtual/EventFd/event_fd_node.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

using namespace fkernel;

static int create_eventfd(uint64_t initval, int flags) {
    auto* task = SchedulerManager::the().current();
    if (!task) return -static_cast<int>(fk::core::Error::IOError);

    auto node_res = EventFdNode::create(static_cast<uint64_t>(initval), flags);
    if (node_res.is_error()) return -static_cast<int>(node_res.error());

    auto dentry_res = Dentry::create("eventfd", nullptr);
    if (dentry_res.is_error()) return -1;
    auto dentry = dentry_res.value();
    dentry->push_node(node_res.value());

    auto desc = fk::make_ref<FileDescription>(dentry, O_RDWR).value();
    int fd = task->add_file_descriptor(desc);
    return fd;
}

// sys_eventfd(...) → 0 or -errno
extern "C" uint64_t sys_eventfd(uint64_t initval, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                     [[maybe_unused]] PtRegs* regs) {
    return static_cast<uint64_t>(create_eventfd(initval, 0));
}
