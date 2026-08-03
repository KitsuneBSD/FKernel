#pragma once

#include <Kernel/Fs/Vfs/Events/kevent.h>
#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Fs/Vfs/Core/timespec.h>
#include <Kernel/Ipc/Notifications/notification.h>
#include <LibFK/Container/Associative/hash_map.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {

struct RegisteredEvent {
    struct kevent event;
    bool enabled{true};
    short last_poll_result{0};
    uint64_t timer_deadline_ticks{0};
    KNoteHook knote;
};

class KQueueNode final : public Node {
public:
    static fk::core::Result<fk::RefPtr<KQueueNode>, fk::core::Error> create();

    KQueueNode() = default;
    virtual ~KQueueNode() override;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override { return fk::core::Error::PermissionDenied; }
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::PermissionDenied; }
    virtual size_t size() const override { return 0; }
    virtual bool is_directory() const override { return false; }
    virtual short poll() const override { return 0; }

    fk::core::Result<int, fk::core::Error> kevent(const struct kevent* changelist, int nchanges,
                                                   struct kevent* eventlist, int nevents,
                                                   const struct timespec* timeout);

    void signal_notification() { m_notification.signal(fk::NotificationBits(1)); }

private:

    void process_changelist(const struct kevent* changelist, int nchanges);
    int scan_ready_events(struct kevent* eventlist, int nevents);
    uint64_t min_timer_deadline() noexcept;

    mutable fk::synchronization::Spinlock m_lock;
    fk::containers::Vector<RegisteredEvent*> m_registered_events;
    fk::containers::HashMap<uint64_t, size_t> m_event_index;
    ipc::Notification m_notification;
    uint64_t m_nearest_timer_deadline{0};
};

} // namespace fkernel
