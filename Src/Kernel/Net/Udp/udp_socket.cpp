#include <Kernel/Net/Udp/udp_socket.h>
#include <Kernel/Net/Udp/udp_header.h>
#include <Kernel/Net/Ip/ipv4_header.h>
#include <Kernel/Net/NetworkStack/network_stack.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibC/string.h>

namespace fkernel {
namespace net {

UdpSocket::UdpSocket() : m_local_port(0), m_remote_ip(IPv4Address::any()) {}

fk::core::Result<void, fk::core::Error> UdpSocket::bind(const char* path) {
    if (!path) return fk::core::Error::InvalidParameter;
    // path is a struct sockaddr_in* cast to const char*
    // Layout: uint16_t sin_family, uint16_t sin_port (network order), uint32_t sin_addr
    const uint16_t* sin_port_ptr = reinterpret_cast<const uint16_t*>(path + 2);
    uint16_t port = ntohs(*sin_port_ptr);
    if (port == 0) return fk::core::Error::InvalidParameter;
    if (!NetworkStack::the().register_udp_socket(port, this))
        return fk::core::Error::AlreadyExists;
    m_local_port = port;
    return {};
}

fk::core::Result<void, fk::core::Error> UdpSocket::connect(const char* path) {
    (void)path;
    return fk::core::Error::NotImplemented;
}

fk::core::Result<void, fk::core::Error> UdpSocket::listen() {
    return fk::core::Error::NotImplemented;
}

fk::core::Result<fk::RefPtr<Socket>, fk::core::Error> UdpSocket::accept() {
    return fk::core::Error::NotImplemented;
}

fk::core::Result<size_t, fk::core::Error> UdpSocket::read(
    [[maybe_unused]] uint64_t offset, size_t size, uint8_t* buf) {
    fk::synchronization::ScopedLock lock(m_lock);
    if (m_recv_buf.is_empty()) return (size_t)0;
    size_t to_copy = size < m_recv_buf.size() ? size : m_recv_buf.size();
    memcpy(buf, m_recv_buf.begin(), to_copy);
    for (size_t i = to_copy; i < m_recv_buf.size(); ++i)
        m_recv_buf[i - to_copy] = m_recv_buf[i];
    for (size_t i = 0; i < to_copy; ++i) m_recv_buf.pop_back();
    return to_copy;
}

fk::core::Result<size_t, fk::core::Error> UdpSocket::write(
    [[maybe_unused]] uint64_t offset, size_t size, const uint8_t* buf) {
    static constexpr size_t MAX_UDP_PAYLOAD = 1472;
    if (size > MAX_UDP_PAYLOAD) return fk::core::Error::InvalidParameter;
    uint8_t packet[UDP_HEADER_SIZE + MAX_UDP_PAYLOAD];
    auto* hdr = reinterpret_cast<UdpHeader*>(packet);
    hdr->fill(m_local_port, m_remote_port, (uint16_t)size);
    memcpy(packet + UDP_HEADER_SIZE, buf, size);
    auto res = NetworkStack::the().send_ipv4(m_remote_ip, IP_PROTO_UDP,
                                              packet, UDP_HEADER_SIZE + size);
    if (res.is_error()) return res.error();
    return size;
}

void UdpSocket::on_receive(IPv4Address src, uint16_t src_port,
                            const uint8_t* data, size_t len) {
    (void)src; (void)src_port;
    fk::synchronization::ScopedLock lock(m_lock);
    for (size_t i = 0; i < len; ++i)
        m_recv_buf.push_back(data[i]);
}

} // namespace net
} // namespace fkernel
