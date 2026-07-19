#pragma once

#include <Kernel/Net/socket.h>
#include <Kernel/Net/unix_socket_buffer.h>
#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {

class UnixSocket final : public Socket {
public:
    static fk::core::Result<fk::RefPtr<UnixSocket>, fk::core::Error> create(SocketType type);

    virtual ~UnixSocket() override;

    virtual SocketDomain domain() const override { return SocketDomain::Unix; }
    virtual SocketType type() const override { return m_type; }

    virtual fk::core::Result<void, fk::core::Error> bind(const char* path) override;
    virtual fk::core::Result<void, fk::core::Error> connect(const char* path) override;
    virtual fk::core::Result<void, fk::core::Error> listen() override;
    virtual fk::core::Result<fk::RefPtr<Socket>, fk::core::Error> accept() override;

    // Node interface
    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    virtual size_t size() const override { return 0; }

    UnixSocket(SocketType type);

private:
    SocketType m_type;
    fk::synchronization::Spinlock m_lock;

    fk::RefPtr<UnixSocket> m_peer;
    fk::RefPtr<UnixSocket> m_backlog[16];
    size_t m_backlog_count{0};
    bool m_listening{false};
    bool m_connected{false};
    Task* m_accept_waiter{nullptr};

    UnixSocketBuffer m_rx_buffer;
};

} // namespace fkernel
