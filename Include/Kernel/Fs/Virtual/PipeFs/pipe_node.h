#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Ipc/notification.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {

class PipeNode final : public Node {
public:
    static fk::core::Result<fk::RefPtr<PipeNode>, fk::core::Error> create();

    PipeNode();
    virtual ~PipeNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    virtual size_t size() const override { return m_buffer.size(); }
    virtual bool is_directory() const override { return false; }
    virtual bool is_pipe() const override { return true; }
    virtual short poll() const override {
        short r = 0;
        m_lock.lock();
        size_t avail = (m_write_pos >= m_read_pos)
            ? (m_write_pos - m_read_pos)
            : (m_buffer.size() - m_read_pos + m_write_pos);
        m_lock.unlock();
        if (avail > 0 || m_eof) r |= POLLIN;
        if (m_reader_count > 0) r |= POLLOUT;
        if (m_reader_count == 0) r |= POLLHUP;
        return r;
    }
    static PipeNode* from_node(Node* n) { return n && n->is_pipe() ? static_cast<PipeNode*>(n) : nullptr; }

    void set_eof() { m_eof = true; m_data_notification.signal(1); }
    void add_reader() { m_reader_count++; }
    void remove_reader() { if (m_reader_count > 0) m_reader_count--; m_space_notification.signal(1); }

    void set_read_nonblock(bool v)  { m_read_nonblock = v; }
    void set_write_nonblock(bool v) { m_write_nonblock = v; }

private:
    static constexpr size_t PIPE_BUFFER_SIZE = 65536;

    fk::containers::Vector<uint8_t> m_buffer;
    size_t m_read_pos{0};
    size_t m_write_pos{0};
    bool m_eof{false};
    size_t m_reader_count{0};
    bool m_read_nonblock{false};
    bool m_write_nonblock{false};

    ipc::Notification m_data_notification;
    ipc::Notification m_space_notification;
    mutable fk::synchronization::Spinlock m_lock;
};

}
