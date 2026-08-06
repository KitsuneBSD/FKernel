#include <Kernel/Fs/Vfs/Events/kqueue.h>
#include <Kernel/Fs/Virtual/Epoll/epoll_node.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

using namespace fkernel;

static constexpr int EPOLL_CTL_ADD = 1;
static constexpr int EPOLL_CTL_DEL = 2;
static constexpr int EPOLL_CTL_MOD = 3;

static constexpr uint32_t EPOLLIN     = 0x001;
static constexpr uint32_t EPOLLOUT    = 0x004;

struct epoll_data_t { uint64_t u64; };
struct epoll_event { uint32_t events; epoll_data_t data; };

// sys_epoll_ctl(...) → 0 or -errno
extern "C" uint64_t sys_epoll_ctl(uint64_t epfd, uint64_t op, uint64_t fd, uint64_t event_ptr,
                        uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return static_cast<uint64_t>(-1);

    auto epoll_desc = task->get_file_descriptor(static_cast<int>(epfd));
    if (!epoll_desc) return return_error(fk::core::Error::InvalidHandle);

    auto node = epoll_desc->node();
    if (!node) return static_cast<uint64_t>(-9);

    auto* epoll = static_cast<EpollNode*>(node.get());
    epoll_event kev{};
    uint32_t events = EPOLLIN | EPOLLOUT;
    uint64_t data_u64 = 0;
    if (event_ptr && fkernel::memory::is_user_address(event_ptr, sizeof(epoll_event))) {
        auto res = fkernel::memory::copy_from_user(&kev, reinterpret_cast<const void*>(event_ptr), sizeof(kev));
        if (res.is_ok()) { events = kev.events; data_u64 = kev.data.u64; }
    }

    if (op == EPOLL_CTL_ADD) epoll->ctl_add(static_cast<int>(fd), events, data_u64);
    if (op == EPOLL_CTL_DEL) epoll->ctl_del(static_cast<int>(fd));
    if (op == EPOLL_CTL_MOD) epoll->ctl_mod(static_cast<int>(fd), events, data_u64);
    return 0;
}
