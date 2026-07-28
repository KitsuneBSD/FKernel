#include <Kernel/Fs/Vfs/definitions.h>
#include <Kernel/Fs/Vfs/kqueue.h>
#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Core/error.h>
#include <LibFK/Types/types.h>

constexpr int FD_SETSIZE = 1024;

struct fd_set {
    unsigned long fds_bits[FD_SETSIZE / (8 * sizeof(unsigned long))];
};

struct timeval {
    long tv_sec;
    long tv_usec;
};

#define FD_ZERO(s) __builtin_memset((s), 0, sizeof(fd_set))
#define FD_SET(fd, s) \
    ((s)->fds_bits[(fd) / (8 * sizeof(unsigned long))] |= (1UL << ((fd) % (8 * sizeof(unsigned long)))))
#define FD_ISSET(fd, s) \
    (((s)->fds_bits[(fd) / (8 * sizeof(unsigned long))] & (1UL << ((fd) % (8 * sizeof(unsigned long))))) != 0)

extern "C" {
uint64_t sys_select(uint64_t nfds_u64, uint64_t readfds_ptr, uint64_t writefds_ptr,
                    uint64_t exceptfds_ptr, uint64_t timeout_ptr, uint64_t,
                    [[maybe_unused]] PtRegs* regs) {
    int nfds = static_cast<int>(nfds_u64);
    auto* task = SchedulerManager::the().current();
    if (!task) return static_cast<uint64_t>(-1);
    if (nfds <= 0) return 0;
    if (nfds > FD_SETSIZE) nfds = FD_SETSIZE;

    fd_set k_readfds{}, k_writefds{};
    bool has_read = false, has_write = false, has_except = false;

    if (readfds_ptr && fkernel::memory::is_user_address(readfds_ptr, sizeof(fd_set))) {
        fkernel::memory::copy_from_user(&k_readfds, reinterpret_cast<const void*>(readfds_ptr), sizeof(fd_set));
        has_read = true;
    }
    if (writefds_ptr && fkernel::memory::is_user_address(writefds_ptr, sizeof(fd_set))) {
        fkernel::memory::copy_from_user(&k_writefds, reinterpret_cast<const void*>(writefds_ptr), sizeof(fd_set));
        has_write = true;
    }
    if (exceptfds_ptr && fkernel::memory::is_user_address(exceptfds_ptr, sizeof(fd_set)))
        has_except = true;

    fd_set orig_read = k_readfds, orig_write = k_writefds;

    bool infinite = (timeout_ptr == 0);
    fkernel::timespec ts;
    const fkernel::timespec* tsp = nullptr;

    if (!infinite && fkernel::memory::is_user_address(timeout_ptr, sizeof(timeval))) {
        timeval tv{};
        fkernel::memory::copy_from_user(&tv, reinterpret_cast<const void*>(timeout_ptr), sizeof(tv));
        if (tv.tv_sec == 0 && tv.tv_usec == 0) {
            ts.tv_sec = 0; ts.tv_nsec = 0; tsp = &ts;
        } else {
            int64_t ms = static_cast<int64_t>(tv.tv_sec) * 1000 + static_cast<int64_t>(tv.tv_usec) / 1000;
            ts.tv_sec  = ms / 1000;
            ts.tv_nsec = (ms % 1000) * 1000000;
            tsp = &ts;
        }
    }

    auto kq_res = fkernel::KQueueNode::create();
    if (kq_res.is_error()) return static_cast<uint64_t>(-12);
    auto kq = kq_res.value();

    int fd_count = 0;
    for (int fd = 0; fd < nfds; ++fd) {
        bool want_r = has_read && FD_ISSET(fd, &orig_read);
        bool want_w = has_write && FD_ISSET(fd, &orig_write);
        if (!want_r && !want_w) continue;

        if (static_cast<size_t>(fd) >= task->resources.files.descriptors.size()) continue;
        auto& desc = task->resources.files.descriptors[fd];
        if (!desc) continue;

        struct fkernel::kevent kev;
        kev.ident  = static_cast<uint64_t>(fd);
        kev.flags  = fkernel::EV_ADD | fkernel::EV_ONESHOT;
        kev.fflags = 0;
        kev.udata  = reinterpret_cast<void*>(static_cast<uintptr_t>(fd));

        if (want_r) {
            kev.filter = fkernel::EVFILT_READ;
            kq->kevent(&kev, 1, nullptr, 0, nullptr);
            fd_count++;
        }
        if (want_w) {
            kev.filter = fkernel::EVFILT_WRITE;
            kq->kevent(&kev, 1, nullptr, 0, nullptr);
            fd_count++;
        }
    }

    if (fd_count == 0) {
        if (has_read)
            fkernel::memory::copy_to_user(reinterpret_cast<void*>(readfds_ptr), &k_readfds, sizeof(fd_set));
        if (has_write)
            fkernel::memory::copy_to_user(reinterpret_cast<void*>(writefds_ptr), &k_writefds, sizeof(fd_set));
        if (has_except) {
            fd_set z{}; FD_ZERO(&z);
            fkernel::memory::copy_to_user(reinterpret_cast<void*>(exceptfds_ptr), &z, sizeof(fd_set));
        }
        return 0;
    }

    int max_kev = fd_count + 1;
    if (max_kev > 2048) max_kev = 2048;
    struct fkernel::kevent kev_buf[2048];

    auto kev_res = kq->kevent(nullptr, 0, kev_buf, max_kev, tsp);
    int n = kev_res.is_ok() ? kev_res.value() : 0;

    fd_set out_read = orig_read;
    fd_set out_write = orig_write;
    int ready = 0;

    for (int i = 0; i < n; ++i) {
        int fd = static_cast<int>(kev_buf[i].ident);
        if (fd < 0 || fd >= nfds) continue;

        if (kev_buf[i].filter == fkernel::EVFILT_READ) {
            if (has_read && FD_ISSET(fd, &orig_read)) {
                FD_SET(fd, &out_read);
                ready++;
            }
        }
        if (kev_buf[i].filter == fkernel::EVFILT_WRITE) {
            if (has_write && FD_ISSET(fd, &orig_write)) {
                FD_SET(fd, &out_write);
                ready++;
            }
        }
    }

    if (has_read)
        fkernel::memory::copy_to_user(reinterpret_cast<void*>(readfds_ptr), &out_read, sizeof(fd_set));
    if (has_write)
        fkernel::memory::copy_to_user(reinterpret_cast<void*>(writefds_ptr), &out_write, sizeof(fd_set));
    if (has_except) {
        fd_set z{}; FD_ZERO(&z);
        fkernel::memory::copy_to_user(reinterpret_cast<void*>(exceptfds_ptr), &z, sizeof(fd_set));
    }

    return static_cast<uint64_t>(ready);
}
}
