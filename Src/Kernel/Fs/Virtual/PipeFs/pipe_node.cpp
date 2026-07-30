#include <Kernel/Fs/Virtual/PipeFs/pipe_node.h>
#include <Kernel/Fs/Vfs/kqueue.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<PipeNode>, fk::core::Error> PipeNode::create() {
    auto res = fk::make_ref<PipeNode>();
    if (!res) return fk::core::Error::OutOfMemory;
    return res.value();
}

PipeNode::PipeNode() {
    m_buffer.resize(PIPE_BUFFER_SIZE);
}

fk::core::Result<size_t, fk::core::Error> PipeNode::read([[maybe_unused]] uint64_t offset, size_t size, uint8_t* buffer) {
    if (size == 0) return 0;

    m_lock.lock();
    while (m_read_pos == m_write_pos && !m_eof) {
        m_lock.unlock();
        if (m_read_nonblock) {
            return fk::core::Error::WouldBlock;
        }
        TRY(m_endpoint.wait_interruptible());
        m_lock.lock();
    }

    if (m_read_pos == m_write_pos && m_eof) {
        m_lock.unlock();
        return 0;
    }

    size_t available = (m_write_pos >= m_read_pos)
                       ? (m_write_pos - m_read_pos)
                       : (PIPE_BUFFER_SIZE - m_read_pos + m_write_pos);
    size_t to_read = (size < available) ? size : available;

    for (size_t i = 0; i < to_read; ++i) {
        buffer[i] = m_buffer[m_read_pos];
        m_read_pos = (m_read_pos + 1) % PIPE_BUFFER_SIZE;
    }
    m_lock.unlock();

    m_endpoint.signal(fk::NotificationBits(1));
    notify_kqueue_writers(this);
    return to_read;
}

fk::core::Result<size_t, fk::core::Error> PipeNode::write([[maybe_unused]] uint64_t offset, size_t size, const uint8_t* buffer) {
    if (size == 0) return 0;
    if (m_reader_count == 0) return fk::core::Error::BrokenPipe;

    size_t total_written = 0;
    m_lock.lock();
    while (total_written < size) {
        if (m_reader_count == 0) {
            m_lock.unlock();
            return fk::core::Error::BrokenPipe;
        }

        size_t used = (m_write_pos >= m_read_pos) ? (m_write_pos - m_read_pos) : (PIPE_BUFFER_SIZE - m_read_pos + m_write_pos);
        size_t space = PIPE_BUFFER_SIZE - used - 1;

        if (space == 0) {
            m_lock.unlock();
            if (m_write_nonblock) {
                return total_written > 0 ? fk::core::Result<size_t, fk::core::Error>(total_written) : fk::core::Error::WouldBlock;
            }
            auto r = m_endpoint.wait_interruptible();
            if (r.is_error()) return r.error();
            m_lock.lock();
            continue;
        }

        size_t remaining = size - total_written;
        size_t to_write = (remaining < space) ? remaining : space;

        for (size_t i = 0; i < to_write; ++i) {
            m_buffer[m_write_pos] = buffer[total_written + i];
            m_write_pos = (m_write_pos + 1) % PIPE_BUFFER_SIZE;
        }

        total_written += to_write;
        m_lock.unlock();
        m_endpoint.signal(fk::NotificationBits(1));
        notify_kqueue_readers(this);
        m_lock.lock();
    }
    m_lock.unlock();

    return total_written;
}

}
