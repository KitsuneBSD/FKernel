#include <LibFK/Utilities/memory.h>

#include <Kernel/Net/Sockets/inet_socket.h>
#include <Kernel/Net/Sockets/udp_socket.h>
#include <Kernel/Net/Sockets/tcp_socket.h>

namespace fkernel {
namespace net {

fk::core::Result<fk::RefPtr<Socket>, fk::core::Error>
create_inet_socket(SocketType type) {
    if (type == SocketType::Datagram) {
        auto res = fk::make_ref<UdpSocket>();
        if (res.is_error()) return res.error();
        return fk::RefPtr<Socket>(res.value());
    }
    if (type == SocketType::Stream) {
        TcpEndpoint local{IPv4Address::any(), 0};
        TcpEndpoint remote{IPv4Address::any(), 0};
        auto res = fk::make_ref<TcpSocket>(local, remote);
        if (res.is_error()) return res.error();
        return fk::RefPtr<Socket>(res.value());
    }
    return fk::core::Error::InvalidParameter;
}

} // namespace net
} // namespace fkernel
