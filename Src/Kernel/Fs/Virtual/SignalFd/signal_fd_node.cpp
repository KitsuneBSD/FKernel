#include <LibFK/Memory/Pointers/ref_ptr.h>

#include <Kernel/Fs/Virtual/SignalFd/signal_fd_node.h>
#include <Kernel/Fs/Vfs/Events/kqueue.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<SignalFdNode>, fk::core::Error>
SignalFdNode::create(Task* task, uint64_t mask) {
    return fk::RefPtr<SignalFdNode>(new SignalFdNode(task, mask));
}

SignalFdNode::SignalFdNode(Task* task, uint64_t mask)
    : m_task(task), m_mask(mask) {}

bool SignalFdNode::try_enqueue(int signum) {
    if (signum < 1 || signum > 64)
        return false;
    uint64_t bit = static_cast<uint64_t>(1) << (signum - 1);
    if ((m_mask & bit) == 0)
        return false;

    m_lock.lock();
    if (m_queue.size() >= MAX_QUEUE) {
        m_lock.unlock();
        return false;
    }
    SignalfdSiginfo info{};
    info.ssi_signo = static_cast<uint32_t>(signum);
    TRY_OR_FATAL(m_queue.push_back(info));
    m_lock.unlock();

    m_endpoint.signal(fk::NotificationBits(1));
    notify_kqueue_readers(this);
    return true;
}

void SignalFdNode::update_mask(uint64_t mask) {
    fk::synchronization::ScopedLock lock(m_lock);
    m_mask = mask;
}

fk::core::Result<size_t, fk::core::Error>
SignalFdNode::read(uint64_t, size_t size, uint8_t* buffer) {
    if (size < sizeof(SignalfdSiginfo))
        return fk::core::Error::InvalidParameter;

    m_lock.lock();
    while (m_queue.size() == 0) {
        m_lock.unlock();
        if (m_nonblock)
            return fk::core::Error::WouldBlock;
        TRY(m_endpoint.wait_interruptible());
        m_lock.lock();
    }
    SignalfdSiginfo info = m_queue[0];
    m_queue.remove_at(0);
    m_lock.unlock();

    __builtin_memcpy(buffer, &info, sizeof(SignalfdSiginfo));
    return sizeof(SignalfdSiginfo);
}

short SignalFdNode::poll() const {
    m_lock.lock();
    bool has_data = m_queue.size() > 0;
    m_lock.unlock();
    return has_data ? (POLLIN | POLLOUT) : POLLOUT;
}

} // namespace fkernel
