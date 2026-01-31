#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Core/Result.h>
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

    virtual bool is_socket() const { return true; }
    virtual SocketDomain domain() const = 0;
    virtual SocketType type() const = 0;

    virtual fk::core::Result<void, fk::core::Error> bind(const char* path) = 0;
    virtual fk::core::Result<void, fk::core::Error> connect(const char* path) = 0;
    virtual fk::core::Result<void, fk::core::Error> listen() = 0;
    virtual fk::core::Result<fk::RefPtr<Socket>, fk::core::Error> accept() = 0;

protected:
    Socket() = default;
};

} // namespace fkernel
