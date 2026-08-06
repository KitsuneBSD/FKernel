#include <Kernel/Fs/Vfs/Events/kqueue.h>
#include <Kernel/Fs/Virtual/Epoll/epoll_node.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

// sys_epoll_create1(...) → 0 or -errno
extern "C" uint64_t sys_epoll_create1(uint64_t flags, uint64_t, uint64_t, uint64_t,
                            uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    (void)flags;
    auto* task = SchedulerManager::the().current();
    if (!task) return static_cast<uint64_t>(-1);

    auto node_res = fk::make_ref<EpollNode>();
    if (node_res.is_error()) return static_cast<uint64_t>(-12);

    auto dentry_res = Dentry::create("epoll", nullptr);
    if (dentry_res.is_error()) return static_cast<uint64_t>(-12);

    auto dentry = dentry_res.value();
    dentry->push_node(node_res.value());

    auto desc_res = fk::make_ref<FileDescription>(dentry, 0);
    if (desc_res.is_error()) return static_cast<uint64_t>(-12);

    return static_cast<uint64_t>(task->add_file_descriptor(desc_res.value()));
}
