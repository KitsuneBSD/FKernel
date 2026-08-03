#include <Kernel/Fs/Vfs/Core/definitions.h>
#include <Kernel/Fs/Vfs/Events/kqueue.h>
#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/error.h>
#include <LibFK/Types/types.h>

struct pollfd {
    int fd;
    short events;
    short revents;
};

extern "C" {
uint64_t sys_poll(uint64_t fds_ptr, uint64_t nfds_u64, uint64_t timeout_raw,
                  uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    int nfds = static_cast<int>(nfds_u64);
    auto* task = SchedulerManager::the().current();
    if (!task || !fds_ptr || nfds <= 0) return static_cast<uint64_t>(-1);
    if (nfds > 1024) nfds = 1024;

    pollfd kfds[1024];
    if (!fkernel::memory::is_user_address(fds_ptr, static_cast<size_t>(nfds) * sizeof(pollfd)))
        return static_cast<uint64_t>(-14);
    auto copy_res = fkernel::memory::copy_from_user(kfds,
        reinterpret_cast<const void*>(fds_ptr), static_cast<size_t>(nfds) * sizeof(pollfd));
    if (copy_res.is_error()) return static_cast<uint64_t>(-14);

    int implicit_ready = 0;
    for (int i = 0; i < nfds; ++i) {
        kfds[i].revents = 0;
        int fd = kfds[i].fd;
        if (fd < 0) {
            kfds[i].revents = POLLNVAL;
            implicit_ready++;
            continue;
        }
        if (static_cast<size_t>(fd) >= task->resources.files.descriptors.size()) {
            kfds[i].revents = POLLNVAL;
            implicit_ready++;
            continue;
        }
        auto& desc = task->resources.files.descriptors[fd];
        if (!desc) {
            kfds[i].revents = POLLNVAL;
            implicit_ready++;
        }
    }

    auto kq_res = fkernel::KQueueNode::create();
    if (kq_res.is_error()) return static_cast<uint64_t>(-12);
    auto kq = kq_res.value();

    for (int i = 0; i < nfds; ++i) {
        if (kfds[i].fd < 0 || kfds[i].revents == POLLNVAL) continue;
        short want = kfds[i].events;

        struct fkernel::kevent kev;
        kev.ident  = static_cast<uint64_t>(kfds[i].fd);
        kev.flags  = fkernel::EV_ADD | fkernel::EV_ONESHOT;
        kev.fflags = 0;
        kev.udata  = reinterpret_cast<void*>(static_cast<uintptr_t>(i));

        if (want & POLLIN) {
            kev.filter = fkernel::EVFILT_READ;
            kq->kevent(&kev, 1, nullptr, 0, nullptr);
        }
        if (want & POLLOUT) {
            kev.filter = fkernel::EVFILT_WRITE;
            kq->kevent(&kev, 1, nullptr, 0, nullptr);
        }
    }

    int64_t timeout_ms = static_cast<int64_t>(static_cast<int32_t>(timeout_raw));
    fkernel::timespec ts;
    const fkernel::timespec* tsp = nullptr;
    if (timeout_ms < 0) {
        tsp = nullptr;
    } else if (timeout_ms == 0) {
        ts.tv_sec = 0; ts.tv_nsec = 0; tsp = &ts;
    } else {
        ts.tv_sec  = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000;
        tsp = &ts;
    }

    int max_kev = nfds * 2;
    if (max_kev < 1) max_kev = 1;
    if (max_kev > 2048) max_kev = 2048;
    struct fkernel::kevent kev_buf[2048];

    auto kev_res = kq->kevent(nullptr, 0, kev_buf, max_kev, tsp);
    int n = kev_res.is_ok() ? kev_res.value() : 0;

    for (int i = 0; i < n; ++i) {
        int idx = static_cast<int>(reinterpret_cast<uintptr_t>(kev_buf[i].udata));
        if (idx < 0 || idx >= nfds) continue;
        if (kev_buf[i].filter == fkernel::EVFILT_READ)  kfds[idx].revents |= POLLIN;
        if (kev_buf[i].filter == fkernel::EVFILT_WRITE) kfds[idx].revents |= POLLOUT;
    }

    fkernel::memory::copy_to_user(reinterpret_cast<void*>(fds_ptr), kfds,
                                   static_cast<size_t>(nfds) * sizeof(pollfd));

    int total_ready = implicit_ready;
    for (int i = 0; i < nfds; ++i) {
        if (kfds[i].fd >= 0 && kfds[i].revents != POLLNVAL && kfds[i].revents != 0)
            total_ready++;
    }
    return static_cast<uint64_t>(total_ready);
}
}
