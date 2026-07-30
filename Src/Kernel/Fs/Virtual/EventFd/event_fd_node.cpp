#include <Kernel/Fs/Virtual/EventFd/event_fd_node.h>
#include <Kernel/Fs/Vfs/kqueue.h>
#include <LibFK/Memory/ref_ptr.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<EventFdNode>, fk::core::Error>
EventFdNode::create(uint64_t initial_value, int flags) {
    if (initial_value > MAX_COUNTER)
        return fk::core::Error::InvalidParameter;
    bool semaphore = (flags & EFD_SEMAPHORE) != 0;
    bool nonblock  = (flags & EFD_NONBLOCK) != 0;
    return fk::RefPtr<EventFdNode>(new EventFdNode(initial_value, semaphore, nonblock));
}

EventFdNode::EventFdNode(uint64_t initial_value, bool semaphore, bool nonblock)
    : m_counter(initial_value), m_is_semaphore(semaphore), m_nonblock(nonblock) {}

fk::core::Result<size_t, fk::core::Error>
EventFdNode::read(uint64_t, size_t size, uint8_t* buffer) {
    if (size < sizeof(uint64_t))
        return fk::core::Error::InvalidParameter;

    m_lock.lock();
    while (m_counter == 0) {
        m_lock.unlock();
        if (m_nonblock)
            return fk::core::Error::WouldBlock;
        TRY(m_endpoint.wait_interruptible());
        m_lock.lock();
    }

    uint64_t val;
    if (m_is_semaphore) {
        val = 1;
        --m_counter;
    } else {
        val = m_counter;
        m_counter = 0;
    }
    m_lock.unlock();

    __builtin_memcpy(buffer, &val, sizeof(uint64_t));
    return sizeof(uint64_t);
}

fk::core::Result<size_t, fk::core::Error>
EventFdNode::write(uint64_t, size_t size, const uint8_t* buffer) {
    if (size < sizeof(uint64_t))
        return fk::core::Error::InvalidParameter;

    uint64_t val;
    __builtin_memcpy(&val, buffer, sizeof(uint64_t));

    if (val == 0xFFFFFFFFFFFFFFFFULL)
        return fk::core::Error::InvalidParameter;

    m_lock.lock();
    if (MAX_COUNTER - m_counter < val) {
        m_lock.unlock();
        return fk::core::Error::WouldBlock;
    }
    m_counter += val;
    m_lock.unlock();

    m_endpoint.signal(fk::NotificationBits(1));
    notify_kqueue_readers(this);
    return sizeof(uint64_t);
}

short EventFdNode::poll() const {
    m_lock.lock();
    bool has_data = (m_counter > 0);
    m_lock.unlock();
    short r = POLLOUT;
    if (has_data)
        r |= POLLIN;
    return r;
}

}
