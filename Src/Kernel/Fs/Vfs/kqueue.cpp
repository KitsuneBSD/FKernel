#include <Kernel/Fs/Vfs/kqueue.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/container_algorithms.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<KQueueNode>, fk::core::Error> KQueueNode::create() {
    auto res = fk::make_ref<KQueueNode>();
    if (!res) return fk::core::Error::OutOfMemory;
    return res.value();
}

KQueueNode::~KQueueNode() {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    auto* current = SchedulerManager::the().current();
    for (auto* reg : m_registered_events) {
        if (!reg) continue;
        if (current) {
            int fd = static_cast<int>(reg->event.ident);
            if (fd >= 0 && static_cast<size_t>(fd) < current->resources.files.descriptors.size()) {
                auto& desc = current->resources.files.descriptors[fd];
                if (desc) {
                    auto node = desc->node();
                    if (node) node->detach_knote(&reg->knote);
                }
            }
        }
        delete reg;
    }
    m_registered_events.clear();
}

void KQueueNode::process_changelist(const struct kevent* changelist, int nchanges) {
    if (nchanges <= 0) return;
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    auto* current = SchedulerManager::the().current();

    for (int i = 0; i < nchanges; ++i) {
        const auto& change = changelist[i];

        if (change.flags & EV_ADD) {
            auto* reg = new RegisteredEvent();
            reg->event = change;
            reg->enabled = !(change.flags & EV_DISABLE);
            reg->last_poll_result = 0;

            int fd = static_cast<int>(change.ident);
            reg->knote.kq     = this;
            reg->knote.ident  = change.ident;
            reg->knote.filter = change.filter;

            if (current && fd >= 0 && static_cast<size_t>(fd) < current->resources.files.descriptors.size()) {
                auto& desc = current->resources.files.descriptors[fd];
                if (desc) {
                    auto node = desc->node();
                    if (node) node->attach_knote(&reg->knote);
                }
            }

            m_registered_events.push_back(reg);
            continue;
        }

        if (change.flags & EV_DELETE) {
            size_t idx = fk::algorithms::find_if(m_registered_events.begin(), m_registered_events.size(),
                [&change](const auto* r) { return r->event.ident == change.ident && r->event.filter == change.filter; });
            if (idx == m_registered_events.size()) continue;

            auto* reg = m_registered_events[idx];
            int fd = static_cast<int>(reg->event.ident);
            if (current && fd >= 0 && static_cast<size_t>(fd) < current->resources.files.descriptors.size()) {
                auto& desc = current->resources.files.descriptors[fd];
                if (desc) {
                    auto node = desc->node();
                    if (node) node->detach_knote(&reg->knote);
                }
            }

            delete reg;
            m_registered_events[idx] = m_registered_events[m_registered_events.size() - 1];
            m_registered_events.pop_back();
            continue;
        }

        if (change.flags & EV_ENABLE) {
            for (auto* reg : m_registered_events) {
                if (reg->event.ident == change.ident && reg->event.filter == change.filter) {
                    reg->enabled = true;
                }
            }
        }

        if (change.flags & EV_DISABLE) {
            for (auto* reg : m_registered_events) {
                if (reg->event.ident == change.ident && reg->event.filter == change.filter) {
                    reg->enabled = false;
                }
            }
        }
    }
}

int KQueueNode::scan_ready_events(struct kevent* eventlist, int nevents) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    auto* current = SchedulerManager::the().current();
    if (!current) return 0;

    int triggered = 0;
    for (size_t i = 0; i < m_registered_events.size() && triggered < nevents; ) {
        auto* reg = m_registered_events[i];
        if (!reg->enabled) { ++i; continue; }

        int fd = static_cast<int>(reg->event.ident);
        if (fd < 0 || static_cast<size_t>(fd) >= current->resources.files.descriptors.size()) {
            ++i; continue;
        }

        auto& desc = current->resources.files.descriptors[fd];
        if (!desc) { ++i; continue; }

        auto node = desc->node();
        if (!node) { ++i; continue; }

        short poll_result = node->poll();
        bool event_triggered = false;
        short relevant_bits = 0;

        if (reg->event.filter == EVFILT_READ) {
            relevant_bits = poll_result & (POLLIN | POLLERR | POLLHUP);
            if (relevant_bits) {
                if (reg->event.flags & EV_CLEAR) {
                    short new_bits = relevant_bits & ~reg->last_poll_result;
                    if (new_bits) event_triggered = true;
                    reg->last_poll_result = poll_result;
                } else {
                    event_triggered = true;
                }
            } else {
                reg->last_poll_result = 0;
            }
        } else if (reg->event.filter == EVFILT_WRITE) {
            relevant_bits = poll_result & (POLLOUT | POLLERR | POLLHUP);
            if (relevant_bits) {
                if (reg->event.flags & EV_CLEAR) {
                    short new_bits = relevant_bits & ~reg->last_poll_result;
                    if (new_bits) event_triggered = true;
                    reg->last_poll_result = poll_result;
                } else {
                    event_triggered = true;
                }
            } else {
                reg->last_poll_result = 0;
            }
        } else if (reg->event.filter == EVFILT_VNODE) {
            event_triggered = (poll_result & (POLLERR | POLLHUP)) != 0;
        } else {
            event_triggered = (poll_result != 0);
        }

        if (event_triggered) {
            reg->event.data = 0;
            reg->event.flags &= ~(EV_ADD | EV_DELETE | EV_ENABLE | EV_DISABLE | EV_RECEIPT);
            if (relevant_bits & POLLHUP) reg->event.flags |= EV_EOF;
            eventlist[triggered] = reg->event;
            triggered++;

            if (reg->event.flags & EV_ONESHOT) {
                auto node_fd = desc->node();
                if (node_fd) node_fd->detach_knote(&reg->knote);
                delete reg;
                m_registered_events[i] = m_registered_events[m_registered_events.size() - 1];
                m_registered_events.pop_back();
                continue;
            }
            if (reg->event.flags & EV_DISPATCH) {
                reg->enabled = false;
            }
        }
        ++i;
    }
    return triggered;
}

fk::core::Result<int, fk::core::Error> KQueueNode::kevent(const struct kevent* changelist, int nchanges,
                                                            struct kevent* eventlist, int nevents,
                                                            const struct timespec* timeout) {
    process_changelist(changelist, nchanges);

    if (nevents <= 0) return 0;

    bool no_wait = false;
    bool infinite = (timeout == nullptr);
    uint64_t deadline = 0;

    if (timeout) {
        if (timeout->tv_sec == 0 && timeout->tv_nsec == 0) {
            no_wait = true;
        } else {
            uint32_t freq = TickManager::the().get_frequency();
            if (freq == 0) freq = 1000;
            uint64_t timeout_ms = static_cast<uint64_t>(timeout->tv_sec) * 1000ULL
                                + static_cast<uint64_t>(timeout->tv_nsec) / 1000000ULL;
            uint64_t timeout_ticks = timeout_ms * freq / 1000;
            if (timeout_ticks == 0) timeout_ticks = 1;
            deadline = TickManager::the().get_ticks() + timeout_ticks;
        }
    }

    while (true) {
        int ready = scan_ready_events(eventlist, nevents);
        if (ready > 0 || no_wait) return ready;
        if (!infinite && TickManager::the().get_ticks() >= deadline) return 0;

        uint64_t wait_ticks;
        if (infinite) {
            uint32_t freq = TickManager::the().get_frequency();
            wait_ticks = (freq > 0) ? freq / 10 : 100;
        } else {
            uint64_t now = TickManager::the().get_ticks();
            if (deadline <= now) return 0;
            wait_ticks = deadline - now;
        }

        m_notification.wait_timeout(fk::TickCount(wait_ticks));
    }
}

void notify_kqueue_readers(Node* node) {
    if (!node) return;
    fk::synchronization::ScopedLockIRQ lock(node->knotes_lock());
    for (auto& knote : node->knotes()) {
        if (knote.filter == EVFILT_READ && knote.kq) {
            knote.kq->signal_notification();
        }
    }
}

void notify_kqueue_writers(Node* node) {
    if (!node) return;
    fk::synchronization::ScopedLockIRQ lock(node->knotes_lock());
    for (auto& knote : node->knotes()) {
        if (knote.filter == EVFILT_WRITE && knote.kq) {
            knote.kq->signal_notification();
        }
    }
}

} // namespace fkernel
