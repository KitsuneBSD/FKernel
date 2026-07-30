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

static void detach_reg(KQueueNode::RegisteredEvent* reg, Task* current) {
    if (reg->event.filter == EVFILT_PROC) {
        auto target = SchedulerManager::the().find_task(fk::ProcessId(reg->event.ident));
        if (target) {
            fk::synchronization::ScopedLockIRQ lock(target->resources.ipc.proc_knotes_lock);
            target->resources.ipc.proc_knotes.remove(&reg->knote);
        }
        return;
    }
    if (reg->event.filter == EVFILT_SIGNAL) {
        if (current) {
            fk::synchronization::ScopedLockIRQ lock(current->resources.ipc.signal_knotes_lock);
            current->resources.ipc.signal_knotes.remove(&reg->knote);
        }
        return;
    }
    if (reg->event.filter == EVFILT_TIMER) return; // no attachment
    // EVFILT_READ/WRITE/VNODE: attached to a VFS node via fd
    if (!current) return;
    int fd = static_cast<int>(reg->event.ident);
    if (fd < 0 || static_cast<size_t>(fd) >= current->resources.files.descriptors.size()) return;
    auto& desc = current->resources.files.descriptors[fd];
    if (!desc) return;
    auto node = desc->node();
    if (node) node->detach_knote(&reg->knote);
}

KQueueNode::~KQueueNode() {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    auto* current = SchedulerManager::the().current();
    for (auto* reg : m_registered_events) {
        if (!reg) continue;
        detach_reg(reg, current);
        delete reg;
    }
    m_registered_events.clear();
}

static uint64_t compute_timer_deadline(const kevent& change) {
    uint32_t freq = TickManager::the().get_frequency();
    if (freq == 0) freq = 1000;
    uint64_t ms = (change.fflags & NOTE_SECONDS)
        ? static_cast<uint64_t>(change.data) * 1000ULL
        : static_cast<uint64_t>(change.data); // default: ms
    uint64_t ticks = ms * freq / 1000;
    return TickManager::the().get_ticks() + (ticks ? ticks : 1);
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
            reg->knote.kq     = this;
            reg->knote.ident  = change.ident;
            reg->knote.filter = change.filter;

            if (change.filter == EVFILT_PROC) {
                auto target = SchedulerManager::the().find_task(fk::ProcessId(change.ident));
                if (target) {
                    fk::synchronization::ScopedLockIRQ tlock(target->resources.ipc.proc_knotes_lock);
                    target->resources.ipc.proc_knotes.push_back(&reg->knote);
                }
            } else if (change.filter == EVFILT_SIGNAL) {
                if (current) {
                    fk::synchronization::ScopedLockIRQ tlock(current->resources.ipc.signal_knotes_lock);
                    current->resources.ipc.signal_knotes.push_back(&reg->knote);
                }
            } else if (change.filter == EVFILT_TIMER) {
                reg->timer_deadline_ticks = compute_timer_deadline(change);
            } else {
                int fd = static_cast<int>(change.ident);
                if (current && fd >= 0 && static_cast<size_t>(fd) < current->resources.files.descriptors.size()) {
                    auto& desc = current->resources.files.descriptors[fd];
                    if (desc) {
                        auto node = desc->node();
                        if (node) node->attach_knote(&reg->knote);
                    }
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
            detach_reg(reg, current);
            delete reg;
            m_registered_events[idx] = m_registered_events[m_registered_events.size() - 1];
            m_registered_events.pop_back();
            continue;
        }

        if (change.flags & EV_ENABLE) {
            for (auto* reg : m_registered_events) {
                if (reg->event.ident == change.ident && reg->event.filter == change.filter)
                    reg->enabled = true;
            }
        }

        if (change.flags & EV_DISABLE) {
            for (auto* reg : m_registered_events) {
                if (reg->event.ident == change.ident && reg->event.filter == change.filter)
                    reg->enabled = false;
            }
        }
    }
}

static bool deliver_event(KQueueNode::RegisteredEvent* reg, kevent* out, Task* current) {
    auto& ev = reg->event;

    if (ev.filter == EVFILT_PROC) {
        if (!reg->knote.pending_fflags) return false;
        *out = ev;
        out->fflags = reg->knote.pending_fflags;
        out->flags &= ~(EV_ADD | EV_DELETE | EV_ENABLE | EV_DISABLE | EV_RECEIPT);
        reg->knote.pending_fflags = 0;
        return true;
    }

    if (ev.filter == EVFILT_SIGNAL) {
        if (!reg->knote.pending_fflags) return false;
        *out = ev;
        out->data = static_cast<int64_t>(reg->knote.pending_fflags);
        out->flags &= ~(EV_ADD | EV_DELETE | EV_ENABLE | EV_DISABLE | EV_RECEIPT);
        reg->knote.pending_fflags = 0;
        return true;
    }

    if (ev.filter == EVFILT_TIMER) {
        if (TickManager::the().get_ticks() < reg->timer_deadline_ticks) return false;
        *out = ev;
        out->flags &= ~(EV_ADD | EV_DELETE | EV_ENABLE | EV_DISABLE | EV_RECEIPT);
        // Reload periodic timer (if fflags has NOTE_MSECONDS/NOTE_SECONDS, it repeats).
        reg->timer_deadline_ticks = compute_timer_deadline(ev);
        return true;
    }

    // EVFILT_READ / EVFILT_WRITE / EVFILT_VNODE: poll-based
    if (!current) return false;
    int fd = static_cast<int>(ev.ident);
    if (fd < 0 || static_cast<size_t>(fd) >= current->resources.files.descriptors.size()) return false;
    auto& desc = current->resources.files.descriptors[fd];
    if (!desc) return false;
    auto node = desc->node();
    if (!node) return false;

    short poll_result = node->poll();
    bool triggered = false;
    short relevant_bits = 0;

    if (ev.filter == EVFILT_READ) {
        relevant_bits = poll_result & (POLLIN | POLLERR | POLLHUP);
        if (relevant_bits) {
            if (ev.flags & EV_CLEAR) {
                triggered = (relevant_bits & ~reg->last_poll_result) != 0;
                reg->last_poll_result = poll_result;
            } else {
                triggered = true;
            }
        } else {
            reg->last_poll_result = 0;
        }
    } else if (ev.filter == EVFILT_WRITE) {
        relevant_bits = poll_result & (POLLOUT | POLLERR | POLLHUP);
        if (relevant_bits) {
            if (ev.flags & EV_CLEAR) {
                triggered = (relevant_bits & ~reg->last_poll_result) != 0;
                reg->last_poll_result = poll_result;
            } else {
                triggered = true;
            }
        } else {
            reg->last_poll_result = 0;
        }
    } else if (ev.filter == EVFILT_VNODE) {
        triggered = (poll_result & (POLLERR | POLLHUP)) != 0;
    } else {
        triggered = (poll_result != 0);
    }

    if (!triggered) return false;
    *out = ev;
    out->data = 0;
    out->flags &= ~(EV_ADD | EV_DELETE | EV_ENABLE | EV_DISABLE | EV_RECEIPT);
    if (relevant_bits & POLLHUP) out->flags |= EV_EOF;
    return true;
}

int KQueueNode::scan_ready_events(struct kevent* eventlist, int nevents) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    auto* current = SchedulerManager::the().current();

    int triggered = 0;
    for (size_t i = 0; i < m_registered_events.size() && triggered < nevents; ) {
        auto* reg = m_registered_events[i];
        if (!reg->enabled) { ++i; continue; }

        if (!deliver_event(reg, &eventlist[triggered], current)) { ++i; continue; }
        triggered++;

        if (reg->event.flags & EV_ONESHOT) {
            detach_reg(reg, current);
            delete reg;
            m_registered_events[i] = m_registered_events[m_registered_events.size() - 1];
            m_registered_events.pop_back();
            continue;
        }
        if (reg->event.flags & EV_DISPATCH) reg->enabled = false;
        ++i;
    }
    return triggered;
}

// Returns the nearest EVFILT_TIMER deadline in ticks, or 0 if none registered.
static uint64_t nearest_timer_deadline(const fk::containers::Vector<KQueueNode::RegisteredEvent*>& events) {
    uint64_t nearest = 0;
    for (auto* reg : events) {
        if (!reg->enabled || reg->event.filter != EVFILT_TIMER) continue;
        if (nearest == 0 || reg->timer_deadline_ticks < nearest)
            nearest = reg->timer_deadline_ticks;
    }
    return nearest;
}

fk::core::Result<int, fk::core::Error> KQueueNode::kevent(const struct kevent* changelist, int nchanges,
                                                            struct kevent* eventlist, int nevents,
                                                            const struct timespec* timeout) {
    process_changelist(changelist, nchanges);
    if (nevents <= 0) return 0;

    bool no_wait = false;
    bool infinite = (timeout == nullptr);
    uint64_t user_deadline = 0;

    if (timeout) {
        if (timeout->tv_sec == 0 && timeout->tv_nsec == 0) {
            no_wait = true;
        } else {
            uint32_t freq = TickManager::the().get_frequency();
            if (freq == 0) freq = 1000;
            uint64_t timeout_ms = static_cast<uint64_t>(timeout->tv_sec) * 1000ULL
                                + static_cast<uint64_t>(timeout->tv_nsec) / 1000000ULL;
            uint64_t ticks = timeout_ms * freq / 1000;
            user_deadline = TickManager::the().get_ticks() + (ticks ? ticks : 1);
        }
    }

    while (true) {
        int ready = scan_ready_events(eventlist, nevents);
        if (ready > 0 || no_wait) return ready;

        uint64_t now = TickManager::the().get_ticks();
        if (!infinite && now >= user_deadline) return 0;

        // Compute wait: min(user timeout, nearest EVFILT_TIMER deadline).
        uint64_t wait_until = infinite ? 0 : user_deadline;
        {
            fk::synchronization::ScopedLockIRQ lock(m_lock);
            uint64_t timer_dl = nearest_timer_deadline(m_registered_events);
            if (timer_dl > 0) {
                if (wait_until == 0 || timer_dl < wait_until) wait_until = timer_dl;
            }
        }

        uint64_t wait_ticks;
        if (wait_until == 0) {
            uint32_t freq = TickManager::the().get_frequency();
            wait_ticks = (freq > 0) ? freq / 10 : 100;
        } else {
            now = TickManager::the().get_ticks();
            wait_ticks = (wait_until > now) ? (wait_until - now) : 1;
        }

        m_notification.wait_timeout(fk::TickCount(wait_ticks));
    }
}

void notify_kqueue_readers(Node* node) {
    if (!node) return;
    fk::synchronization::ScopedLockIRQ lock(node->knotes_lock());
    for (auto& knote : node->knotes()) {
        if (knote.filter == EVFILT_READ && knote.kq)
            knote.kq->signal_notification();
    }
}

void notify_kqueue_writers(Node* node) {
    if (!node) return;
    fk::synchronization::ScopedLockIRQ lock(node->knotes_lock());
    for (auto& knote : node->knotes()) {
        if (knote.filter == EVFILT_WRITE && knote.kq)
            knote.kq->signal_notification();
    }
}

void notify_proc_kqueue(Task* task, uint32_t fflags) {
    if (!task) return;
    fk::synchronization::ScopedLockIRQ lock(task->resources.ipc.proc_knotes_lock);
    for (auto& knote : task->resources.ipc.proc_knotes) {
        knote.pending_fflags |= fflags;
        if (knote.kq) knote.kq->signal_notification();
    }
}

void notify_signal_kqueue(Task* task, int signum) {
    if (!task) return;
    fk::synchronization::ScopedLockIRQ lock(task->resources.ipc.signal_knotes_lock);
    for (auto& knote : task->resources.ipc.signal_knotes) {
        // Only signal watchers registered for this specific signal number.
        if (knote.ident == static_cast<uint64_t>(signum)) {
            knote.pending_fflags |= 1;
            if (knote.kq) knote.kq->signal_notification();
        }
    }
}

} // namespace fkernel
