#pragma once

#include <Kernel/Ipc/notification.h>
#include <Kernel/Net/Ip/ip_address.h>
#include <Kernel/Net/Tcp/tcp_header.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace net {

enum class TcpState : uint8_t {
    Closed, Listen, SynSent, SynReceived,
    Established, FinWait1, FinWait2,
    CloseWait, LastAck, TimeWait, Closing,
};

struct TcpEndpoint {
    IPv4Address ip;
    uint16_t    port;
};

static constexpr size_t TCP_RECV_BUFFER_SIZE = 65536;

class TcpConnection {
  TcpEndpoint m_local;
  TcpEndpoint m_remote;

public:
  TcpState             state{TcpState::Closed};
  uint32_t             send_next{0};
  uint32_t             recv_next{0};
  uint16_t             send_window{65535};   // advertised to peer
  uint16_t             peer_window{65535};   // peer's advertised window
  uint32_t             send_unacked{0};      // oldest unacknowledged seq
  fk::containers::Vector<uint8_t> recv_buf;
  fkernel::ipc::Notification state_changed;

  uint16_t recv_window() const {
    size_t used = recv_buf.size();
    if (used >= TCP_RECV_BUFFER_SIZE) return 0;
    size_t avail = TCP_RECV_BUFFER_SIZE - used;
    return static_cast<uint16_t>(avail > 65535 ? 65535 : avail);
  }

  TcpConnection(TcpEndpoint local, TcpEndpoint remote);
  const TcpEndpoint& local()  const { return m_local; }
  const TcpEndpoint& remote() const { return m_remote; }
  void set_local_port(uint16_t port) { m_local.port = port; }
  void set_remote(TcpEndpoint ep) { m_remote = ep; }

  bool matches(uint32_t src_ip, uint16_t src_port,
               uint32_t dst_ip, uint16_t dst_port) const;
};

} // namespace net
} // namespace fkernel
