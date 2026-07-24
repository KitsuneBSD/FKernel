#include <Kernel/Fs/Vfs/kqueue.h>
#include <Kernel/Scheduler/scheduler.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<KQueueNode>, fk::core::Error> KQueueNode::create() {
    auto res = fk::make_ref<KQueueNode>();
    if (!res) return fk::core::Error::OutOfMemory;
    return res.value();
}

fk::core::Result<int, fk::core::Error> KQueueNode::kevent(const struct kevent* changelist, int nchanges,
                                                        [[maybe_unused]] struct kevent* eventlist, int nevents) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    for (int i = 0; i < nchanges; ++i) {
        const auto& change = changelist[i];
        if (change.flags & EV_ADD) {
            m_registered_events.push_back({change, !(change.flags & EV_DISABLE)});
        }
        if (change.flags & EV_DELETE) {
            for (size_t j = 0; j < m_registered_events.size(); ++j) {
                if (m_registered_events[j].event.ident == change.ident && 
                    m_registered_events[j].event.filter == change.filter) {
                    // Primitive removal
                    m_registered_events[j] = m_registered_events[m_registered_events.size() - 1];
                    m_registered_events.pop_back();
                    break;
                }
            }
        }
    }

    // Wait for events if eventlist is provided
    if (nevents > 0) {
        int triggered = 0;
        auto* current = SchedulerManager::the().current();
        if (!current) return 0;

        for (const auto& reg : m_registered_events) {
            if (!reg.enabled) continue;
            if (triggered >= nevents) break;

            int fd = static_cast<int>(reg.event.ident);
            // Basic validation of FD against current process
            if (fd < 0 || static_cast<size_t>(fd) >= current->resources.files.descriptors.size()) continue;

            auto& desc = current->resources.files.descriptors[fd];
            if (!desc) continue;

            auto node = desc->node();
            if (!node) continue;

            short events = node->poll();
            bool event_triggered = false;

            // Check filters
            if (reg.event.filter == EVFILT_READ && (events & POLLIN)) {
                event_triggered = true;
            } else if (reg.event.filter == EVFILT_WRITE && (events & POLLOUT)) {
                event_triggered = true;
            }

            if (event_triggered) {
                eventlist[triggered] = reg.event;
                // TODO: Set .data to bytes available if possible
                triggered++;
            }
        }
        return triggered;
    }

    return 0;
}

} // namespace fkernel
