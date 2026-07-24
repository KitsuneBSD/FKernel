#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Core/result.h>
#include <LibFK/Memory/ref_ptr.h>

namespace fkernel {

enum class SocketDomain {
    Unix = 1,
    Inet = 2,
};

enum class SocketType {
    Stream = 1,
    Datagram = 2,
};

class Socket : public Node {
public:
    virtual ~Socket() override = default;

    virtual bool is_socket() const override { return true; }
    virtual SocketDomain domain() const = 0;
    virtual SocketType type() const = 0;

    virtual fk::core::Result<void, fk::core::Error> bind(const char* path) = 0;
    virtual fk::core::Result<void, fk::core::Error> connect(const char* path) = 0;
    virtual fk::core::Result<void, fk::core::Error> listen() = 0;
    virtual fk::core::Result<fk::RefPtr<Socket>, fk::core::Error> accept() = 0;

    virtual fk::core::Result<size_t, fk::core::Error> sendto(
        size_t size, const uint8_t* buf,
        [[maybe_unused]] const char* addr, [[maybe_unused]] uint32_t addrlen) {
        return write(0, size, buf);
    }

    virtual fk::core::Result<size_t, fk::core::Error> recvfrom(
        size_t size, uint8_t* buf,
        [[maybe_unused]] char* addr, [[maybe_unused]] uint32_t* addrlen) {
        return read(0, size, buf);
    }

    virtual fk::core::Result<void, fk::core::Error> shutdown([[maybe_unused]] int how) {
        return fk::core::Error::NotImplemented;
    }

    virtual fk::core::Result<void, fk::core::Error> getsockname(
        [[maybe_unused]] char* addr, [[maybe_unused]] uint32_t* addrlen) {
        return fk::core::Error::NotImplemented;
    }

    virtual fk::core::Result<void, fk::core::Error> getpeername(
        [[maybe_unused]] char* addr, [[maybe_unused]] uint32_t* addrlen) {
        return fk::core::Error::NotImplemented;
    }

    virtual fk::core::Result<void, fk::core::Error> setsockopt(
        [[maybe_unused]] int level, [[maybe_unused]] int optname,
        [[maybe_unused]] const void* optval, [[maybe_unused]] uint32_t optlen) {
        return fk::core::Error::NotImplemented;
    }

    virtual fk::core::Result<void, fk::core::Error> getsockopt(
        [[maybe_unused]] int level, [[maybe_unused]] int optname,
        [[maybe_unused]] void* optval, [[maybe_unused]] uint32_t* optlen) {
        return fk::core::Error::NotImplemented;
    }

protected:
    Socket() = default;
};

} // namespace fkernel
