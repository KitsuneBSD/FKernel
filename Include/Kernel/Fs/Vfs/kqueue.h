#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Ipc/notification.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {

struct kevent {
    uint64_t ident;
    int16_t filter;
    uint16_t flags;
    uint32_t fflags;
    int64_t data;
    void* udata;
};

// Filters
constexpr int16_t EVFILT_READ = -1;
constexpr int16_t EVFILT_WRITE = -2;

// Flags
constexpr uint16_t EV_ADD = 0x0001;
constexpr uint16_t EV_DELETE = 0x0002;
constexpr uint16_t EV_ENABLE = 0x0004;
constexpr uint16_t EV_DISABLE = 0x0008;
constexpr uint16_t EV_CLEAR = 0x0020;

class KQueueNode final : public Node {
public:
    static fk::core::Result<fk::RefPtr<KQueueNode>, fk::core::Error> create();

    KQueueNode() = default;
    virtual ~KQueueNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override { return fk::core::Error::PermissionDenied; }
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::PermissionDenied; }
    virtual size_t size() const override { return 0; }
    virtual bool is_directory() const override { return false; }

    fk::core::Result<int, fk::core::Error> kevent(const struct kevent* changelist, int nchanges,
                                                struct kevent* eventlist, int nevents);

private:
    struct RegisteredEvent {
        struct kevent event;
        bool enabled{true};
    };

    mutable fk::synchronization::Spinlock m_lock;
    fk::containers::Vector<RegisteredEvent> m_registered_events;
    ipc::Notification m_notification;
};

} // namespace fkernel
