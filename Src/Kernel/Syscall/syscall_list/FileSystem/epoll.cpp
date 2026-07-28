#include <Kernel/Fs/Vfs/kqueue.h>
#include <Kernel/Fs/Virtual/Epoll/epoll_node.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
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

extern "C" {

uint64_t sys_epoll_create1(uint64_t flags, uint64_t, uint64_t, uint64_t,
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

uint64_t sys_epoll_ctl(uint64_t epfd, uint64_t op, uint64_t fd, uint64_t event_ptr,
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

uint64_t sys_epoll_wait(uint64_t epfd, uint64_t events_ptr, uint64_t maxevents,
                         uint64_t timeout, uint64_t, uint64_t,
                         [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return static_cast<uint64_t>(-1);

    auto epoll_desc = task->get_file_descriptor(static_cast<int>(epfd));
    if (!epoll_desc) return return_error(fk::core::Error::InvalidHandle);

    auto node = epoll_desc->node();
    if (!node) return static_cast<uint64_t>(-9);

    auto* epoll = static_cast<EpollNode*>(node.get());
    auto kq = epoll->kqueue_node();

    int max = static_cast<int>(maxevents);
    if (max <= 0 || max > 128) max = 128;
    if (!fkernel::memory::is_user_address(events_ptr, static_cast<size_t>(max) * sizeof(epoll_event)))
        return static_cast<uint64_t>(-14);

    int64_t timeout_ms = static_cast<int64_t>(static_cast<int32_t>(timeout));

    timespec ts;
    const timespec* tsp = nullptr;
    if (timeout_ms >= 0) {
        ts.tv_sec  = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000;
        tsp = &ts;
    }

    int max_kev = max * 2;
    if (max_kev > 256) max_kev = 256;
    struct kevent kev_buf[256];

    auto kev_res = kq->kevent(nullptr, 0, kev_buf, max_kev, tsp);
    int n = kev_res.is_ok() ? kev_res.value() : 0;
    if (n < 0) return static_cast<uint64_t>(n);

    bool seen[1024] = {};
    int out_count = 0;
    for (int i = 0; i < n && out_count < max; ++i) {
        int efd = static_cast<int>(kev_buf[i].ident);
        if (efd < 0 || efd >= 1024) continue;
        if (seen[efd]) continue;
        seen[efd] = true;

        uint32_t epoll_flags = 0;
        for (int j = i; j < n; ++j) {
            if (static_cast<int>(kev_buf[j].ident) != efd) continue;
            if (kev_buf[j].filter == EVFILT_READ)  epoll_flags |= EPOLLIN;
            if (kev_buf[j].filter == EVFILT_WRITE) epoll_flags |= EPOLLOUT;
        }

        epoll_event out_ev;
        out_ev.events   = epoll_flags;
        out_ev.data.u64 = reinterpret_cast<uint64_t>(kev_buf[i].udata);
        fkernel::memory::copy_to_user(
            reinterpret_cast<void*>(events_ptr + out_count * sizeof(epoll_event)),
            &out_ev, sizeof(out_ev));
        out_count++;
    }

    return static_cast<uint64_t>(out_count);
}

}
