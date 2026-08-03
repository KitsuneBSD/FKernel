#include <Kernel/Net/Tcp/tcp_connection.h>
#include <Kernel/Net/Core/byte_order.h>

namespace fkernel {
namespace net {

TcpConnection::TcpConnection(TcpEndpoint local, TcpEndpoint remote)
    : m_local(local), m_remote(remote) {}

bool TcpConnection::matches(uint32_t src_ip, uint16_t src_port,
                             uint32_t dst_ip, uint16_t dst_port) const {
    return m_remote.ip.to_network() == src_ip
        && m_remote.port == ntohs(src_port)
        && m_local.ip.to_network() == dst_ip
        && m_local.port == ntohs(dst_port);
}

} // namespace net
} // namespace fkernel
