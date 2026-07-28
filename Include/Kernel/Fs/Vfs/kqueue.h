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

struct timespec {
    int64_t tv_sec;
    int64_t tv_nsec;
};

// Filters
constexpr int16_t EVFILT_READ   = -1;
constexpr int16_t EVFILT_WRITE  = -2;
constexpr int16_t EVFILT_TIMER  = -3;
constexpr int16_t EVFILT_VNODE  = -4;
constexpr int16_t EVFILT_PROC   = -5;
constexpr int16_t EVFILT_SIGNAL = -6;
constexpr int16_t EVFILT_USER   = -7;

// General flags
constexpr uint16_t EV_ADD       = 0x0001;
constexpr uint16_t EV_DELETE    = 0x0002;
constexpr uint16_t EV_ENABLE    = 0x0004;
constexpr uint16_t EV_DISABLE   = 0x0008;
constexpr uint16_t EV_ONESHOT   = 0x0010;
constexpr uint16_t EV_CLEAR     = 0x0020;
constexpr uint16_t EV_RECEIPT   = 0x0040;
constexpr uint16_t EV_DISPATCH  = 0x0080;
constexpr uint16_t EV_EOF       = 0x8000;
constexpr uint16_t EV_ERROR     = 0x4000;

// Return-only flags (set by kernel in kevent.flags)
constexpr uint16_t EV_ADDED     = 0x0100;  // EV_RECEIPT: change was processed

// fflags for EVFILT_VNODE
constexpr uint32_t NOTE_DELETE  = 0x00000001;
constexpr uint32_t NOTE_WRITE   = 0x00000002;
constexpr uint32_t NOTE_EXTEND  = 0x00000004;
constexpr uint32_t NOTE_ATTRIB  = 0x00000008;
constexpr uint32_t NOTE_LINK    = 0x00000010;
constexpr uint32_t NOTE_RENAME  = 0x00000020;
constexpr uint32_t NOTE_REVOKE  = 0x00000040;

// fflags for EVFILT_PROC
constexpr uint32_t NOTE_EXIT    = 0x80000000;
constexpr uint32_t NOTE_FORK    = 0x40000000;
constexpr uint32_t NOTE_EXEC    = 0x20000000;
constexpr uint32_t NOTE_TRACK   = 0x00000001;
constexpr uint32_t NOTE_CHILD   = 0x00000002;

// fflags for EVFILT_TIMER
constexpr uint32_t NOTE_SECONDS = 0x00000001;
constexpr uint32_t NOTE_MSECONDS = 0x00000002;

// fflags fkkernel for EVFILT_USER
constexpr uint32_t NOTE_TRIGGER = 0x01000000;
constexpr uint32_t NOTE_FFNOP   = 0x00000000;

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
    struct RegisteredEvent {
        struct kevent event;
        bool enabled{true};
        short last_poll_result{0};
        KNoteHook knote;
    };

    void process_changelist(const struct kevent* changelist, int nchanges);
    int scan_ready_events(struct kevent* eventlist, int nevents);

    mutable fk::synchronization::Spinlock m_lock;
    fk::containers::Vector<RegisteredEvent*> m_registered_events;
    ipc::Notification m_notification;
};

} // namespace fkernel

namespace fkernel {
    void notify_kqueue_readers(Node* node);
    void notify_kqueue_writers(Node* node);
}
