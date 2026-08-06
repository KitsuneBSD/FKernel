#include <LibFK/Algorithms/Generic/container_algorithms.h>
#include <LibFK/Algorithms/Crypto/internet_checksum.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Net/Sockets/udp_socket.h>
#include <Kernel/Net/Udp/udp_header.h>
#include <Kernel/Net/Ip/ipv4_header.h>
#include <Kernel/Net/Core/network_stack.h>

namespace fkernel {
namespace net {

// RFC 768 pseudo-header checksum over UDP datagram (header + payload)
static uint16_t udp_checksum(IPv4Address dst, const uint8_t* seg, size_t seg_len) {
    struct __attribute__((packed)) {
        uint32_t src_ip;
        uint32_t dst_ip;
        uint8_t  zero;
        uint8_t  proto;
        uint16_t len;
    } ph;
    ph.src_ip = NetworkStack::the().ip().to_network();
    ph.dst_ip = dst.to_network();
    ph.zero   = 0;
    ph.proto  = IP_PROTO_UDP;
    ph.len    = htons((uint16_t)seg_len);
    fk::algorithms::InternetChecksum cs;
    cs.accumulate(&ph, sizeof(ph));
    cs.accumulate(seg, seg_len);
    return cs.finalize();
}

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
    fk::algorithms::kdebug("UDP", "bind(%u)", port);
    return {};
}

fk::core::Result<void, fk::core::Error> UdpSocket::connect(const char* path) {
    if (!path) return fk::core::Error::InvalidParameter;
    // path is sockaddr_in*: sin_family(2) + sin_port(2, BE) + sin_addr(4)
    uint16_t remote_port = ntohs(*reinterpret_cast<const uint16_t*>(path + 2));
    uint32_t remote_ip_be = *reinterpret_cast<const uint32_t*>(path + 4);

    {
        fk::synchronization::ScopedLock lock(m_lock);
        if (m_local_port == 0) {
            // Auto-bind an ephemeral port (Linux semantics: UDP connect implies bind).
            static constexpr uint16_t UDP_EPHEMERAL_START = 49152;
            static uint16_t s_ephemeral{UDP_EPHEMERAL_START};
            uint16_t local_port = __sync_fetch_and_add(&s_ephemeral, 1);
            if (!NetworkStack::the().register_udp_socket(local_port, this))
                return fk::core::Error::AlreadyExists;
            m_local_port = local_port;
        }
        m_remote_ip = IPv4Address(ntohl(remote_ip_be));
        m_remote_port = remote_port;
    }
    fk::algorithms::kdebug("UDP", "connect(%s:%u)",
                           IPv4Address(ntohl(remote_ip_be)).to_string().c_str(), remote_port);
    return {};
}

fk::core::Result<void, fk::core::Error> UdpSocket::listen() {
    // Linux allows listen() on datagram sockets as a no-op (accept() then fails EOPNOTSUPP).
    fk::algorithms::kdebug("UDP", "listen() (no-op for datagram)");
    return {};
}

fk::core::Result<fk::RefPtr<Socket>, fk::core::Error> UdpSocket::accept() {
    return fk::core::Error::NotSupported; // EOPNOTSUPP
}

fk::core::Result<size_t, fk::core::Error> UdpSocket::read(
    [[maybe_unused]] uint64_t offset, size_t size, uint8_t* buf) {
    fk::synchronization::ScopedLock lock(m_lock);
    if (m_recv_queue.is_empty()) return (size_t)0;
    auto& entry = m_recv_queue[0];
    size_t to_copy = size < entry.data.size() ? size : entry.data.size();
    fk::memory::copy(buf, entry.data.begin(), to_copy);
    m_recv_queue.remove_at(0);
    return to_copy;
}

fk::core::Result<size_t, fk::core::Error> UdpSocket::recvfrom(
    size_t size, uint8_t* buf, char* addr, uint32_t* addrlen) {
    fk::synchronization::ScopedLock lock(m_lock);
    if (m_recv_queue.is_empty()) return (size_t)0;
    auto& entry = m_recv_queue[0];
    size_t to_copy = size < entry.data.size() ? size : entry.data.size();
    fk::memory::copy(buf, entry.data.begin(), to_copy);
    if (addr && addrlen && *addrlen >= 16) {
        uint16_t* family = reinterpret_cast<uint16_t*>(addr);
        uint16_t* port   = reinterpret_cast<uint16_t*>(addr + 2);
        uint32_t* ip     = reinterpret_cast<uint32_t*>(addr + 4);
        *family = 2; // AF_INET
        *port   = htons(entry.src_port);
        *ip     = htonl(entry.src_ip);
        *addrlen = 16;
    }
    m_recv_queue.remove_at(0);
    return to_copy;
}

fk::core::Result<size_t, fk::core::Error> UdpSocket::sendto(
    size_t size, const uint8_t* buf, const char* addr, [[maybe_unused]] uint32_t addrlen) {
    if (!addr) return write(0, size, buf);
    uint16_t dst_port = ntohs(*reinterpret_cast<const uint16_t*>(addr + 2));
    uint32_t dst_ip   = ntohl(*reinterpret_cast<const uint32_t*>(addr + 4));
    static constexpr size_t MAX_UDP_PAYLOAD = 1472;
    if (size > MAX_UDP_PAYLOAD) return fk::core::Error::InvalidParameter;
    uint8_t packet[UDP_HEADER_SIZE + MAX_UDP_PAYLOAD];
    auto* hdr = reinterpret_cast<UdpHeader*>(packet);
    hdr->fill(m_local_port, dst_port, (uint16_t)size);
    fk::memory::copy(packet + UDP_HEADER_SIZE, buf, size);
    hdr->checksum = udp_checksum(IPv4Address(dst_ip), packet, UDP_HEADER_SIZE + size);
    auto res = NetworkStack::the().send_ipv4(IPv4Address(dst_ip), IP_PROTO_UDP,
                                              packet, UDP_HEADER_SIZE + size);
    if (res.is_error()) return res.error();
    return size;
}

fk::core::Result<void, fk::core::Error> UdpSocket::getsockname(char* addr, uint32_t* addrlen) {
    if (!addr || !addrlen || *addrlen < 16) return fk::core::Error::InvalidParameter;
    uint16_t* family = reinterpret_cast<uint16_t*>(addr);
    uint16_t* port   = reinterpret_cast<uint16_t*>(addr + 2);
    uint32_t* ip     = reinterpret_cast<uint32_t*>(addr + 4);
    *family  = 2; // AF_INET
    *port    = htons(m_local_port);
    *ip      = htonl(NetworkStack::the().ip().value);
    *addrlen = 16;
    return {};
}

fk::core::Result<void, fk::core::Error> UdpSocket::getpeername(char* addr, uint32_t* addrlen) {
    if (!addr || !addrlen || *addrlen < 16) return fk::core::Error::InvalidParameter;
    if (m_remote_port == 0 && m_remote_ip.value == 0)
        return fk::core::Error::NotConnected;
    uint16_t* family = reinterpret_cast<uint16_t*>(addr);
    uint16_t* port   = reinterpret_cast<uint16_t*>(addr + 2);
    uint32_t* ip     = reinterpret_cast<uint32_t*>(addr + 4);
    *family  = 2; // AF_INET
    *port    = htons(m_remote_port);
    *ip      = htonl(m_remote_ip.value);
    *addrlen = 16;
    return {};
}

fk::core::Result<void, fk::core::Error> UdpSocket::setsockopt(
    int level, int optname, const void* optval, uint32_t optlen) {
    if (level != 1) return fk::core::Error::ProtocolNotSupported; // only SOL_SOCKET=1
    if (!optval || optlen < 4) return fk::core::Error::InvalidParameter;
    int val = *reinterpret_cast<const int*>(optval);
    switch (optname) {
    case 2:  // SO_REUSEADDR
        m_so_reuseaddr = (val != 0);
        return {};
    case 6:  // SO_BROADCAST
        m_so_broadcast = (val != 0);
        return {};
    case 9:  // SO_KEEPALIVE
        m_so_keepalive = (val != 0);
        return {};
    case 7:  // SO_SNDBUF
    case 8:  // SO_RCVBUF
        // Buffers are fixed-size in FKernel; accept and ignore.
        return {};
    default:
        return fk::core::Error::ProtocolNotSupported; // ENOPROTOOPT
    }
}

fk::core::Result<void, fk::core::Error> UdpSocket::getsockopt(
    int level, int optname, void* optval, uint32_t* optlen) {
    if (level != 1) return fk::core::Error::ProtocolNotSupported;
    if (!optval || !optlen || *optlen < 4) return fk::core::Error::InvalidParameter;
    switch (optname) {
    case 3:  // SO_TYPE
        *reinterpret_cast<int*>(optval) = 2; // SOCK_DGRAM
        *optlen = 4;
        return {};
    case 4:  // SO_ERROR
        *reinterpret_cast<int*>(optval) = 0; // no pending error
        *optlen = 4;
        return {};
    case 2:  // SO_REUSEADDR
        *reinterpret_cast<int*>(optval) = m_so_reuseaddr ? 1 : 0;
        *optlen = 4;
        return {};
    case 6:  // SO_BROADCAST
        *reinterpret_cast<int*>(optval) = m_so_broadcast ? 1 : 0;
        *optlen = 4;
        return {};
    case 9:  // SO_KEEPALIVE
        *reinterpret_cast<int*>(optval) = m_so_keepalive ? 1 : 0;
        *optlen = 4;
        return {};
    case 7:  // SO_SNDBUF
    case 8:  // SO_RCVBUF
        *reinterpret_cast<int*>(optval) = 65536;
        *optlen = 4;
        return {};
    default:
        return fk::core::Error::ProtocolNotSupported; // ENOPROTOOPT
    }
}

fk::core::Result<size_t, fk::core::Error> UdpSocket::write(
    [[maybe_unused]] uint64_t offset, size_t size, const uint8_t* buf) {
    static constexpr size_t MAX_UDP_PAYLOAD = 1472;
    if (size > MAX_UDP_PAYLOAD) return fk::core::Error::InvalidParameter;
    uint8_t packet[UDP_HEADER_SIZE + MAX_UDP_PAYLOAD];
    auto* hdr = reinterpret_cast<UdpHeader*>(packet);
    hdr->fill(m_local_port, m_remote_port, (uint16_t)size);
    fk::memory::copy(packet + UDP_HEADER_SIZE, buf, size);
    hdr->checksum = udp_checksum(m_remote_ip, packet, UDP_HEADER_SIZE + size);
    auto res = NetworkStack::the().send_ipv4(m_remote_ip, IP_PROTO_UDP,
                                              packet, UDP_HEADER_SIZE + size);
    if (res.is_error()) return res.error();
    return size;
}

fk::core::Result<void, fk::core::Error> UdpSocket::bind_port(uint16_t port) {
    if (port == 0) return fk::core::Error::InvalidParameter;
    if (!NetworkStack::the().register_udp_socket(port, this))
        return fk::core::Error::AlreadyExists;
    m_local_port = port;
    return {};
}

void UdpSocket::on_receive(IPv4Address src, uint16_t src_port,
                            const uint8_t* data, size_t len) {
    fk::synchronization::ScopedLock lock(m_lock);
    constexpr size_t MAX_RECV_ENTRIES = 64;
    if (m_recv_queue.size() >= MAX_RECV_ENTRIES) {
        fk::algorithms::kwarn("UDP", "Receive queue full, dropping %zu bytes", len);
        return;
    }
    UdpRecvEntry entry;
    entry.src_ip   = src.value;
    entry.src_port = src_port;
    TRY_OR_FATAL(entry.data.push_range(data, len));
    TRY_OR_FATAL(m_recv_queue.push_back(fk::types::move(entry)));
}

} // namespace net
} // namespace fkernel
