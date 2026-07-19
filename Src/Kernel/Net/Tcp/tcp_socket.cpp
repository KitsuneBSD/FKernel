#include <Kernel/Net/Tcp/tcp_socket.h>
#include <Kernel/Net/Ip/ipv4_header.h>
#include <Kernel/Net/NetworkStack/network_stack.h>
#include <Kernel/Net/byte_order.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibC/string.h>

namespace fkernel {
namespace net {

TcpSocket::TcpSocket(TcpEndpoint local, TcpEndpoint remote)
    : m_connection(local, remote) {}

fk::core::Result<void, fk::core::Error> TcpSocket::bind(const char* path) {
    if (!path) return fk::core::Error::InvalidParameter;
    // path is struct sockaddr_in* cast to const char*
    // Layout: uint16_t sin_family, uint16_t sin_port (network order), uint32_t sin_addr
    const uint16_t* sin_port_ptr = reinterpret_cast<const uint16_t*>(path + 2);
    uint16_t port = ntohs(*sin_port_ptr);
    if (port == 0) return fk::core::Error::InvalidParameter;
    if (!NetworkStack::the().register_tcp_socket(port, this))
        return fk::core::Error::AlreadyExists;
    m_connection.set_local_port(port);
    return {};
}

fk::core::Result<void, fk::core::Error> TcpSocket::connect(const char* path) {
    (void)path; return fk::core::Error::NotImplemented;
}

fk::core::Result<void, fk::core::Error> TcpSocket::listen() {
    m_connection.state = TcpState::Listen;
    return {};
}

fk::core::Result<fk::RefPtr<Socket>, fk::core::Error> TcpSocket::accept() {
    return fk::core::Error::NotImplemented;
}

fk::core::Result<size_t, fk::core::Error> TcpSocket::read(
    [[maybe_unused]] uint64_t offset, size_t size, uint8_t* buf) {
    fk::synchronization::ScopedLock lock(m_lock);
    auto& rb = m_connection.recv_buf;
    if (rb.is_empty()) return (size_t)0;
    size_t to_copy = size < rb.size() ? size : rb.size();
    memcpy(buf, rb.begin(), to_copy);
    for (size_t i = to_copy; i < rb.size(); ++i)
        rb[i - to_copy] = rb[i];
    for (size_t i = 0; i < to_copy; ++i) rb.pop_back();
    return to_copy;
}

fk::core::Result<size_t, fk::core::Error> TcpSocket::write(
    [[maybe_unused]] uint64_t offset, size_t size, const uint8_t* buf) {
    fk::synchronization::ScopedLock lock(m_lock);
    if (m_connection.state != TcpState::Established)
        return fk::core::Error::NotImplemented;
    static constexpr size_t MSS = 1460;
    size_t sent = 0;
    while (sent < size) {
        uint16_t wnd = m_connection.peer_window;
        if (wnd == 0) break;
        size_t chunk = size - sent;
        if (chunk > MSS) chunk = MSS;
        if (chunk > wnd) chunk = wnd;
        uint8_t packet[TCP_HEADER_SIZE + MSS];
        auto* hdr = reinterpret_cast<TcpHeader*>(packet);
        hdr->fill(m_connection.local().port, m_connection.remote().port,
                  m_connection.send_next, m_connection.recv_next,
                  TCP_FLAG_ACK | TCP_FLAG_PSH, m_connection.recv_window());
        memcpy(packet + TCP_HEADER_SIZE, buf + sent, chunk);
        auto res = NetworkStack::the().send_ipv4(
            m_connection.remote().ip, IP_PROTO_TCP, packet, TCP_HEADER_SIZE + chunk);
        if (res.is_error()) return res.error();
        m_connection.send_next += (uint32_t)chunk;
        if (m_connection.peer_window >= (uint16_t)chunk)
            m_connection.peer_window -= (uint16_t)chunk;
        else
            m_connection.peer_window = 0;
        sent += chunk;
    }
    return sent;
}

void TcpSocket::on_segment(const TcpHeader* hdr, const uint8_t* data, size_t data_len) {
    fk::synchronization::ScopedLock lock(m_lock);
    uint8_t flags = hdr->flags;
    uint32_t seq = ntohl(hdr->seq_num);

    process_handshake(hdr, flags, seq);
    process_ack(hdr, flags);
    process_data(hdr, flags, data, data_len, seq);
    process_fin(hdr, flags);
}

void TcpSocket::process_handshake(const TcpHeader* hdr, uint8_t flags, uint32_t seq) {
    if (m_connection.state != TcpState::Listen) return;
    if (!(flags & TCP_FLAG_SYN)) return;
    m_connection.recv_next = seq + 1;
    m_connection.peer_window = ntohs(hdr->window);
    m_connection.state = TcpState::SynReceived;
    uint8_t synack[TCP_HEADER_SIZE];
    auto* reply = reinterpret_cast<TcpHeader*>(synack);
    reply->fill(m_connection.local().port, m_connection.remote().port,
                m_connection.send_next, m_connection.recv_next,
                TCP_FLAG_SYN | TCP_FLAG_ACK, m_connection.recv_window());
    NetworkStack::the().send_ipv4(m_connection.remote().ip, IP_PROTO_TCP,
                                  synack, TCP_HEADER_SIZE);
    m_connection.send_unacked = m_connection.send_next;
    ++m_connection.send_next;
}

void TcpSocket::process_ack(const TcpHeader* hdr, uint8_t flags) {
    if (!(flags & TCP_FLAG_ACK)) return;
    if (m_connection.state == TcpState::SynReceived)
        m_connection.state = TcpState::Established;
    uint32_t ack = ntohl(hdr->ack_num);
    if (ack > m_connection.send_unacked && ack <= m_connection.send_next)
        m_connection.send_unacked = ack;
    m_connection.peer_window = ntohs(hdr->window);
}

void TcpSocket::process_data(const TcpHeader*, uint8_t, const uint8_t* data, size_t data_len, uint32_t seq) {
    if (data_len == 0) return;
    if (m_connection.state != TcpState::Established) return;
    if (seq != m_connection.recv_next) return;
    uint16_t avail = m_connection.recv_window();
    size_t to_store = data_len < avail ? data_len : avail;
    for (size_t i = 0; i < to_store; ++i)
        m_connection.recv_buf.push_back(data[i]);
    m_connection.recv_next += (uint32_t)to_store;
    uint8_t ack_pkt[TCP_HEADER_SIZE];
    auto* ack = reinterpret_cast<TcpHeader*>(ack_pkt);
    ack->fill(m_connection.local().port, m_connection.remote().port,
              m_connection.send_next, m_connection.recv_next,
              TCP_FLAG_ACK, m_connection.recv_window());
    NetworkStack::the().send_ipv4(m_connection.remote().ip, IP_PROTO_TCP,
                                  ack_pkt, TCP_HEADER_SIZE);
}

void TcpSocket::process_fin(const TcpHeader*, uint8_t flags) {
    if (!(flags & TCP_FLAG_FIN)) return;
    ++m_connection.recv_next;
    m_connection.state = TcpState::CloseWait;
    uint8_t fin_ack[TCP_HEADER_SIZE];
    auto* fa = reinterpret_cast<TcpHeader*>(fin_ack);
    fa->fill(m_connection.local().port, m_connection.remote().port,
             m_connection.send_next, m_connection.recv_next,
             TCP_FLAG_ACK | TCP_FLAG_FIN, m_connection.recv_window());
    NetworkStack::the().send_ipv4(m_connection.remote().ip, IP_PROTO_TCP,
                                  fin_ack, TCP_HEADER_SIZE);
    ++m_connection.send_next;
    m_connection.state = TcpState::LastAck;
}

} // namespace net
} // namespace fkernel
