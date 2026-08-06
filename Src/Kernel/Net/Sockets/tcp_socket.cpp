#include <LibFK/Algorithms/Crypto/internet_checksum.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Net/Sockets/tcp_socket.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Net/Ip/ipv4_header.h>
#include <Kernel/Net/Core/network_stack.h>
#include <Kernel/Net/Core/byte_order.h>

namespace fkernel {
namespace net {

static constexpr uint16_t TCP_MAX_WINDOW = 65535;

// RFC 793 pseudo-header checksum over TCP segment (header + payload)
static uint16_t tcp_checksum(IPv4Address dst, const uint8_t* seg, size_t seg_len) {
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
    ph.proto  = IP_PROTO_TCP;
    ph.len    = htons((uint16_t)seg_len);
    fk::algorithms::InternetChecksum cs;
    cs.accumulate(&ph, sizeof(ph));
    cs.accumulate(seg, seg_len);
    return cs.finalize();
}

TcpSocket::TcpSocket(TcpEndpoint local, TcpEndpoint remote)
    : m_connection(local, remote) {
  register_socket(this);
}

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
    fk::algorithms::kdebug("TCP", "bind(%u)", port);
    return {};
}

fk::core::Result<void, fk::core::Error> TcpSocket::connect(const char* path) {
    if (!path) return fk::core::Error::InvalidParameter;
    // path is sockaddr_in*: sin_family(2) + sin_port(2, BE) + sin_addr(4)
    uint16_t remote_port = ntohs(*reinterpret_cast<const uint16_t*>(path + 2));
    uint32_t remote_ip_be = *reinterpret_cast<const uint32_t*>(path + 4);
    fk::algorithms::kdebug("TCP", "connect(%s:%u)",
        IPv4Address(ntohl(remote_ip_be)).to_string().c_str(), remote_port);

    {
        fk::synchronization::ScopedLock lock(m_lock);
        static constexpr uint16_t TCP_EPHEMERAL_START = 49152;
        static uint16_t s_ephemeral{TCP_EPHEMERAL_START};
        uint16_t local_port = __sync_fetch_and_add(&s_ephemeral, 1);
        m_connection.set_local_port(local_port);
        m_connection.set_remote({IPv4Address(ntohl(remote_ip_be)), remote_port});
        NetworkStack::the().register_tcp_socket(local_port, this);

        m_connection.send_next = (uint32_t)TickManager::the().get_ticks();
        m_connection.send_unacked = m_connection.send_next;
        m_connection.state = TcpState::SynSent;

        uint8_t syn[TCP_HEADER_SIZE];
        auto* syn_hdr = reinterpret_cast<TcpHeader*>(syn);
        syn_hdr->fill(local_port, remote_port, m_connection.send_next, 0,
                      TCP_FLAG_SYN, TCP_MAX_WINDOW);
        syn_hdr->checksum = tcp_checksum(IPv4Address(ntohl(remote_ip_be)), syn, TCP_HEADER_SIZE);
        ++m_connection.send_next;
        NetworkStack::the().send_ipv4(IPv4Address(ntohl(remote_ip_be)),
                                      IP_PROTO_TCP, syn, TCP_HEADER_SIZE);
    }

    while (m_connection.state == TcpState::SynSent)
        TRY(m_connection.state_changed.wait_interruptible());

    if (m_connection.state != TcpState::Established)
        return fk::core::Error::IOError;
    return {};
}

fk::core::Result<void, fk::core::Error> TcpSocket::listen() {
    m_connection.state = TcpState::Listen;
    fk::algorithms::kdebug("TCP", "listen()");
    return {};
}

fk::core::Result<fk::RefPtr<Socket>, fk::core::Error> TcpSocket::accept() {
    while (true) {
        {
            fk::synchronization::ScopedLock lock(m_lock);
            if (!m_accept_queue.is_empty()) {
                auto child = m_accept_queue[m_accept_queue.size() - 1];
                m_accept_queue.pop_back();
                return fk::RefPtr<Socket>(child);
            }
        }
        TRY(m_connection.state_changed.wait_interruptible());
    }
}

fk::core::Result<size_t, fk::core::Error> TcpSocket::read(
    [[maybe_unused]] uint64_t offset, size_t size, uint8_t* buf) {
    fk::synchronization::ScopedLock lock(m_lock);
    auto& rb = m_connection.recv_buf;
    if (rb.is_empty()) return (size_t)0;
    size_t to_copy = size < rb.size() ? size : rb.size();
    rb.pop_range(buf, to_copy);
    return to_copy;
}

fk::core::Result<size_t, fk::core::Error> TcpSocket::write(
    [[maybe_unused]] uint64_t offset, size_t size, const uint8_t* buf) {
    fk::synchronization::ScopedLock lock(m_lock);
    if (m_connection.state != TcpState::Established)
        return fk::core::Error::NotConnected;
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
        fk::memory::copy(packet + TCP_HEADER_SIZE, buf + sent, chunk);
        hdr->checksum = tcp_checksum(m_connection.remote().ip, packet, TCP_HEADER_SIZE + chunk);
        auto res = NetworkStack::the().send_ipv4(
            m_connection.remote().ip, IP_PROTO_TCP, packet, TCP_HEADER_SIZE + chunk);
        if (res.is_error()) return res.error();
        m_connection.send_next += (uint32_t)chunk;
        if (m_connection.peer_window >= (uint16_t)chunk)
            m_connection.peer_window -= (uint16_t)chunk;
        else
            m_connection.peer_window = 0;
        sent += chunk;

        TRY(m_retransmit_buf.resize(chunk));
        fk::memory::copy(&m_retransmit_buf[0], buf + sent - chunk, chunk);
        m_retransmit_seq = m_connection.send_next - (uint32_t)chunk;
        m_retransmit_len = chunk;
        arm_retransmit();
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
    if (m_connection.state == TcpState::SynSent) {
        if ((flags & (TCP_FLAG_SYN | TCP_FLAG_ACK)) != (TCP_FLAG_SYN | TCP_FLAG_ACK)) {
            fk::algorithms::kwarn("TCP", "handshake failed: state=%d", (int)m_connection.state);
            return;
        }
        m_connection.recv_next = seq + 1;
        m_connection.peer_window = ntohs(hdr->window);
        m_connection.send_unacked = ntohl(hdr->ack_num);
        m_connection.state = TcpState::Established;
        uint8_t ack[TCP_HEADER_SIZE];
        auto* reply = reinterpret_cast<TcpHeader*>(ack);
        reply->fill(m_connection.local().port, m_connection.remote().port,
                    m_connection.send_next, m_connection.recv_next,
                    TCP_FLAG_ACK, m_connection.recv_window());
        reply->checksum = tcp_checksum(m_connection.remote().ip, ack, TCP_HEADER_SIZE);
        NetworkStack::the().send_ipv4(m_connection.remote().ip, IP_PROTO_TCP,
                                      ack, TCP_HEADER_SIZE);
        m_connection.state_changed.signal(fk::NotificationBits(1));
        return;
    }
    if (m_connection.state != TcpState::Listen) return;
    if (!(flags & TCP_FLAG_SYN)) {
        fk::algorithms::kwarn("TCP", "handshake failed: state=%d", (int)m_connection.state);
        return;
    }
    // Create child socket for this connection
    TcpEndpoint any_ep{IPv4Address(0u), 0};
    auto child_res = fk::make_ref<TcpSocket>(m_connection.local(), any_ep);
    if (child_res.is_error()) return;
    auto child = child_res.value();
    child->m_connection.recv_next = seq + 1;
    child->m_connection.peer_window = ntohs(hdr->window);
    child->m_connection.send_next = (uint32_t)TickManager::the().get_ticks();
    child->m_connection.set_remote(m_connection.remote());
    child->m_connection.state = TcpState::SynReceived;

    uint8_t synack[TCP_HEADER_SIZE];
    auto* reply = reinterpret_cast<TcpHeader*>(synack);
    reply->fill(m_connection.local().port, m_connection.remote().port,
                child->m_connection.send_next, child->m_connection.recv_next,
                TCP_FLAG_SYN | TCP_FLAG_ACK, child->m_connection.recv_window());
    reply->checksum = tcp_checksum(m_connection.remote().ip, synack, TCP_HEADER_SIZE);
    child->m_connection.send_unacked = child->m_connection.send_next;
    ++child->m_connection.send_next;
    NetworkStack::the().send_ipv4(m_connection.remote().ip, IP_PROTO_TCP,
                                  synack, TCP_HEADER_SIZE);

    // Parent stays registered; process_ack will complete the handshake via m_pending
    TRY_OR_FATAL(m_pending.push_back(child));
}

void TcpSocket::process_ack(const TcpHeader* hdr, uint8_t flags) {
    if (!(flags & TCP_FLAG_ACK)) return;
    uint32_t ack = ntohl(hdr->ack_num);

    if (m_connection.state == TcpState::Listen) {
        // Complete the 3-way handshake for the matching SynReceived child.
        for (size_t i = 0; i < m_pending.size(); ++i) {
            auto& child = m_pending[i];
            if (child->m_connection.state != TcpState::SynReceived) continue;
            if (ack != child->m_connection.send_next) continue;
            child->m_connection.state = TcpState::Established;
            child->m_connection.send_unacked = ack;
            child->m_connection.peer_window = ntohs(hdr->window);
            // Swap-remove from pending O(1), push to ready queue.
            auto ready = child;
            if (i != m_pending.size() - 1)
                m_pending[i] = m_pending[m_pending.size() - 1];
            m_pending.pop_back();
            TRY_OR_FATAL(m_accept_queue.push_back(ready));
            m_connection.state_changed.signal(fk::NotificationBits(1));
            return;
        }
        return;
    }

    if (m_connection.state == TcpState::SynReceived) {
        m_connection.state = TcpState::Established;
        m_connection.state_changed.signal(fk::NotificationBits(1));
    }
    if (ack > m_connection.send_unacked && ack <= m_connection.send_next) {
        m_connection.send_unacked = ack;
        if (m_retransmit_len > 0 && ack >= m_retransmit_seq + (uint32_t)m_retransmit_len)
            cancel_retransmit();
    }
    m_connection.peer_window = ntohs(hdr->window);
}

void TcpSocket::process_data(const TcpHeader*, uint8_t, const uint8_t* data, size_t data_len, uint32_t seq) {
    if (data_len == 0) return;
    if (m_connection.state != TcpState::Established) return;
    if (seq != m_connection.recv_next) return;
    uint16_t avail = m_connection.recv_window();
    size_t to_store = data_len < avail ? data_len : avail;
    m_connection.recv_buf.push_range(data, to_store);
    m_connection.recv_next += (uint32_t)to_store;
    uint8_t ack_pkt[TCP_HEADER_SIZE];
    auto* ack = reinterpret_cast<TcpHeader*>(ack_pkt);
    ack->fill(m_connection.local().port, m_connection.remote().port,
              m_connection.send_next, m_connection.recv_next,
              TCP_FLAG_ACK, m_connection.recv_window());
    ack->checksum = tcp_checksum(m_connection.remote().ip, ack_pkt, TCP_HEADER_SIZE);
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
    fa->checksum = tcp_checksum(m_connection.remote().ip, fin_ack, TCP_HEADER_SIZE);
    NetworkStack::the().send_ipv4(m_connection.remote().ip, IP_PROTO_TCP,
                                  fin_ack, TCP_HEADER_SIZE);
    ++m_connection.send_next;
    m_connection.state = TcpState::LastAck;
}

fk::core::Result<void, fk::core::Error> TcpSocket::shutdown(int how) {
    fk::synchronization::ScopedLock lock(m_lock);
    if (m_connection.state != TcpState::Established &&
        m_connection.state != TcpState::CloseWait)
        return fk::core::Error::InvalidParameter;
    if (how == 0 || how == 2) // SHUT_RD or SHUT_RDWR
        m_connection.recv_buf.clear();
    if (how == 1 || how == 2) { // SHUT_WR or SHUT_RDWR
        uint8_t fin[TCP_HEADER_SIZE];
        auto* hdr = reinterpret_cast<TcpHeader*>(fin);
        hdr->fill(m_connection.local().port, m_connection.remote().port,
                  m_connection.send_next, m_connection.recv_next,
                  TCP_FLAG_FIN | TCP_FLAG_ACK, m_connection.recv_window());
        hdr->checksum = tcp_checksum(m_connection.remote().ip, fin, TCP_HEADER_SIZE);
        NetworkStack::the().send_ipv4(m_connection.remote().ip, IP_PROTO_TCP,
                                      fin, TCP_HEADER_SIZE);
        ++m_connection.send_next;
        m_connection.state = TcpState::FinWait1;
    }
    return {};
}

fk::core::Result<void, fk::core::Error> TcpSocket::getsockname(char* addr, uint32_t* addrlen) {
    if (!addr || !addrlen || *addrlen < 16) return fk::core::Error::InvalidParameter;
    *reinterpret_cast<uint16_t*>(addr)     = 2; // AF_INET
    *reinterpret_cast<uint16_t*>(addr + 2) = htons(m_connection.local().port);
    *reinterpret_cast<uint32_t*>(addr + 4) = htonl(m_connection.local().ip.value);
    *addrlen = 16;
    return {};
}

fk::core::Result<void, fk::core::Error> TcpSocket::getpeername(char* addr, uint32_t* addrlen) {
    if (!addr || !addrlen || *addrlen < 16) return fk::core::Error::InvalidParameter;
    if (m_connection.state != TcpState::Established)
        return fk::core::Error::InvalidParameter;
    *reinterpret_cast<uint16_t*>(addr)     = 2; // AF_INET
    *reinterpret_cast<uint16_t*>(addr + 2) = htons(m_connection.remote().port);
    *reinterpret_cast<uint32_t*>(addr + 4) = htonl(m_connection.remote().ip.value);
    *addrlen = 16;
    return {};
}

fk::core::Result<void, fk::core::Error> TcpSocket::setsockopt(
    int level, int optname, const void* optval, uint32_t optlen) {
    if (!optval || optlen < 4) return fk::core::Error::InvalidParameter;
    int val = *reinterpret_cast<const int*>(optval);
    if (level == 1) { // SOL_SOCKET
        if (optname == 2) { m_so_reuseaddr = (val != 0); return {}; } // SO_REUSEADDR
        if (optname == 9) { m_so_keepalive = (val != 0); return {}; }  // SO_KEEPALIVE
        return fk::core::Error::ProtocolNotSupported;
    }
    if (level == 6) { // IPPROTO_TCP
        if (optname == 1) { m_tcp_nodelay = (val != 0); return {}; } // TCP_NODELAY
        return {};
    }
    return fk::core::Error::ProtocolNotSupported;
}

fk::core::Result<void, fk::core::Error> TcpSocket::getsockopt(
    int level, int optname, void* optval, uint32_t* optlen) {
    if (!optval || !optlen || *optlen < 4) return fk::core::Error::InvalidParameter;
    if (level == 1) { // SOL_SOCKET
        if (optname == 4) { *reinterpret_cast<int*>(optval) = m_so_error; *optlen = 4; return {}; } // SO_ERROR
        if (optname == 2) { *reinterpret_cast<int*>(optval) = m_so_reuseaddr ? 1 : 0; *optlen = 4; return {}; } // SO_REUSEADDR
        if (optname == 9) { *reinterpret_cast<int*>(optval) = m_so_keepalive ? 1 : 0; *optlen = 4; return {}; } // SO_KEEPALIVE
        return fk::core::Error::ProtocolNotSupported;
    }
    if (level == 6) { // IPPROTO_TCP
        if (optname == 1) { *reinterpret_cast<int*>(optval) = m_tcp_nodelay ? 1 : 0; *optlen = 4; return {}; } // TCP_NODELAY
        return fk::core::Error::ProtocolNotSupported;
    }
    return fk::core::Error::ProtocolNotSupported;
}

} // namespace net
} // namespace fkernel

#include <LibFK/Container/Sequence/vector.h>

namespace fkernel {
namespace net {

static fk::containers::Vector<TcpSocket*>& tcp_socket_list() {
  static fk::containers::Vector<TcpSocket*> list;
  return list;
}

void TcpSocket::register_socket(TcpSocket* s) {
  TRY_OR_FATAL(tcp_socket_list().push_back(s));
}

void TcpSocket::unregister_socket(TcpSocket* s) {
  auto& list = tcp_socket_list();
  for (size_t i = 0; i < list.size(); ++i) {
    if (list[i] == s) {
      list[i] = list[list.size() - 1];
      list.pop_back();
      return;
    }
  }
}

void TcpSocket::tick_all(uint64_t now_ticks) {
  for (auto* s : tcp_socket_list())
    s->on_tick(now_ticks);
}

void TcpSocket::arm_retransmit() {
  m_connection.retransmit_ticks = TickManager::the().get_ticks() + TcpConnection::RTO_TICKS;
}

void TcpSocket::cancel_retransmit() {
  m_connection.retransmit_ticks = 0;
  m_connection.retransmit_count = 0;
}

void TcpSocket::do_retransmit() {
  if (m_retransmit_len == 0) return;

  fk::containers::Vector<uint8_t> packet;
  TRY_OR_FATAL(packet.resize(TCP_HEADER_SIZE + m_retransmit_len));
  auto* hdr = reinterpret_cast<TcpHeader*>(&packet[0]);
  hdr->fill(m_connection.local().port, m_connection.remote().port,
            m_retransmit_seq, m_connection.recv_next,
            TCP_FLAG_ACK | TCP_FLAG_PSH, m_connection.recv_window());
  fk::memory::copy(&packet[0] + TCP_HEADER_SIZE, &m_retransmit_buf[0], m_retransmit_len);
  hdr->checksum = tcp_checksum(m_connection.remote().ip, &packet[0],
                                TCP_HEADER_SIZE + m_retransmit_len);
  auto res = NetworkStack::the().send_ipv4(m_connection.remote().ip, IP_PROTO_TCP,
                                           &packet[0], TCP_HEADER_SIZE + m_retransmit_len);
  if (res.is_ok()) {
    ++m_connection.retransmit_count;
    m_connection.retransmit_ticks = TickManager::the().get_ticks()
        + TcpConnection::RTO_TICKS * (1ULL << m_connection.retransmit_count);
    fk::algorithms::kwarn("TCP", "Retransmit seq=%u len=%zu attempt=%u",
                          m_retransmit_seq, m_retransmit_len, m_connection.retransmit_count);
  }
}

void TcpSocket::on_tick(uint64_t now_ticks) {
  if (m_connection.retransmit_ticks == 0) return;
  if (now_ticks < m_connection.retransmit_ticks) return;
  if (m_connection.retransmit_count >= TcpConnection::MAX_RETRANSMITS) {
    m_connection.state = TcpState::Closed;
    m_connection.retransmit_ticks = 0;
    m_connection.state_changed.signal(fk::NotificationBits(1));
    return;
  }
  do_retransmit();
}

}
}
