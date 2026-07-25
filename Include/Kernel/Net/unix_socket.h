#pragma once

#include <Kernel/Net/socket.h>
#include <Kernel/Net/unix_socket_buffer.h>
#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Text/fixed_string.h>

namespace fkernel {

class UnixSocket final : public Socket {
public:
    static fk::core::Result<fk::RefPtr<UnixSocket>, fk::core::Error> create(SocketType type);
    static fk::core::Result<void, fk::core::Error> create_pair(
        SocketType type, fk::RefPtr<UnixSocket>& a, fk::RefPtr<UnixSocket>& b);

    virtual ~UnixSocket() override;

    virtual SocketDomain domain() const override { return SocketDomain::Unix; }
    virtual SocketType type() const override { return m_type; }

    virtual fk::core::Result<void, fk::core::Error> bind(const char* path) override;
    virtual fk::core::Result<void, fk::core::Error> connect(const char* path) override;
    virtual fk::core::Result<void, fk::core::Error> listen() override;
    virtual fk::core::Result<fk::RefPtr<Socket>, fk::core::Error> accept() override;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    virtual size_t size() const override { return 0; }

    virtual fk::core::Result<void, fk::core::Error> getsockname(char* addr, uint32_t* addrlen) override;
    virtual fk::core::Result<void, fk::core::Error> getpeername(char* addr, uint32_t* addrlen) override;
    virtual short poll() const override {
        short r = 0;
        if (m_rx_buffer.available() > 0 || (m_listening && m_backlog_count > 0)) r |= POLLIN;
        if (m_connected && m_rx_buffer.free_space() > 0) r |= POLLOUT;
        if (!m_connected && !m_listening) r |= POLLHUP;
        return r;
    }

    UnixSocket(SocketType type);

private:
    SocketType m_type;
    fk::text::fixed_string<108> m_bound_path;
    fk::synchronization::Spinlock m_lock;

    UnixSocket* m_peer{nullptr};
    fk::RefPtr<UnixSocket> m_backlog[16];
    size_t m_backlog_count{0};
    bool m_listening{false};
    bool m_connected{false};
    Task* m_accept_waiter{nullptr};

    UnixSocketBuffer m_rx_buffer;
};

} // namespace fkernel
