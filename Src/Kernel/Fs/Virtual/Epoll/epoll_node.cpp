#include <Kernel/Fs/Virtual/Epoll/epoll_node.h>

namespace fkernel {

bool EpollNode::ctl_add(int fd, uint32_t events, uint64_t data) {
    for (auto& e : m_entries) {
        if (e.fd == fd) return false;
    }

    if (!m_kq) {
        auto kq_res = KQueueNode::create();
        if (kq_res.is_error()) return false;
        m_kq = kq_res.value();
    }

    TRY_OR_FATAL(m_entries.push_back({fd, events, data}));

    struct kevent kev;
    kev.ident  = static_cast<uint64_t>(fd);
    kev.flags  = EV_ADD;
    kev.fflags = 0;
    kev.udata  = reinterpret_cast<void*>(static_cast<uintptr_t>(data));
    kev.data   = 0;

    if (events & 0x001) { // EPOLLIN
        kev.filter = EVFILT_READ;
        m_kq->kevent(&kev, 1, nullptr, 0, nullptr);
    }
    if (events & 0x004) { // EPOLLOUT
        kev.filter = EVFILT_WRITE;
        m_kq->kevent(&kev, 1, nullptr, 0, nullptr);
    }

    return true;
}

bool EpollNode::ctl_del(int fd) {
    for (size_t i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].fd != fd) continue;

        if (m_kq) {
            uint32_t ev = m_entries[i].events;
            struct kevent kev;
            kev.ident  = static_cast<uint64_t>(fd);
            kev.flags  = EV_DELETE;
            kev.fflags = 0;
            kev.udata  = nullptr;
            kev.data   = 0;

            if (ev & 0x001) {
                kev.filter = EVFILT_READ;
                m_kq->kevent(&kev, 1, nullptr, 0, nullptr);
            }
            if (ev & 0x004) {
                kev.filter = EVFILT_WRITE;
                m_kq->kevent(&kev, 1, nullptr, 0, nullptr);
            }
        }

        m_entries[i] = m_entries[m_entries.size() - 1];
        m_entries.pop_back();
        return true;
    }
    return false;
}

bool EpollNode::ctl_mod(int fd, uint32_t events, uint64_t data) {
    for (auto& e : m_entries) {
        if (e.fd != fd) continue;

        if (m_kq) {
            uint32_t old_ev = e.events;
            struct kevent kev;
            kev.ident  = static_cast<uint64_t>(fd);
            kev.fflags = 0;
            kev.udata  = nullptr;
            kev.data   = 0;

            if (old_ev & 0x001) {
                kev.filter = EVFILT_READ;
                kev.flags  = EV_DELETE;
                m_kq->kevent(&kev, 1, nullptr, 0, nullptr);
            }
            if (old_ev & 0x004) {
                kev.filter = EVFILT_WRITE;
                kev.flags  = EV_DELETE;
                m_kq->kevent(&kev, 1, nullptr, 0, nullptr);
            }

            kev.flags  = EV_ADD;
            kev.udata  = reinterpret_cast<void*>(static_cast<uintptr_t>(data));

            if (events & 0x001) {
                kev.filter = EVFILT_READ;
                m_kq->kevent(&kev, 1, nullptr, 0, nullptr);
            }
            if (events & 0x004) {
                kev.filter = EVFILT_WRITE;
                m_kq->kevent(&kev, 1, nullptr, 0, nullptr);
            }
        }

        e.events   = events;
        e.data_u64 = data;
        return true;
    }
    return false;
}

} // namespace fkernel
