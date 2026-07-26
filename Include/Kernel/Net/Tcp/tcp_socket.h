#pragma once

#include <Kernel/Net/Ip/ip_address.h>
#include <Kernel/Net/Tcp/tcp_connection.h>
#include <Kernel/Net/socket.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace net {

class TcpSocket : public Socket {
  TcpConnection m_connection;
  fk::synchronization::Spinlock m_lock;
  fk::containers::Vector<fk::RefPtr<TcpSocket>> m_accept_queue;

  fk::containers::Vector<uint8_t> m_retransmit_buf;
  uint32_t m_retransmit_seq{0};
  size_t   m_retransmit_len{0};

public:
  TcpSocket(TcpEndpoint local, TcpEndpoint remote);

  SocketDomain domain() const override { return SocketDomain::Inet; }
  SocketType   type()   const override { return SocketType::Stream; }

  fk::core::Result<void, fk::core::Error> bind(const char* path) override;
  fk::core::Result<void, fk::core::Error> connect(const char* path) override;
  fk::core::Result<void, fk::core::Error> listen() override;
  fk::core::Result<fk::RefPtr<Socket>, fk::core::Error> accept() override;

  fk::core::Result<size_t, fk::core::Error> read(
      uint64_t offset, size_t size, uint8_t* buf) override;
  fk::core::Result<size_t, fk::core::Error> write(
      uint64_t offset, size_t size, const uint8_t* buf) override;

  fk::core::Result<void, fk::core::Error> shutdown(int how) override;
  fk::core::Result<void, fk::core::Error> getsockname(char* addr, uint32_t* addrlen) override;
  fk::core::Result<void, fk::core::Error> getpeername(char* addr, uint32_t* addrlen) override;
  fk::core::Result<void, fk::core::Error> setsockopt(
      int level, int optname, const void* optval, uint32_t optlen) override;
  fk::core::Result<void, fk::core::Error> getsockopt(
      int level, int optname, void* optval, uint32_t* optlen) override;

  size_t size() const override { return 0; }
  bool is_directory() const override { return false; }
  short poll() const override {
    short r = 0;
    if (!m_connection.recv_buf.is_empty()) r |= POLLIN;
    if (!m_accept_queue.is_empty()) r |= POLLIN;
    if (m_connection.state == TcpState::Established) r |= POLLOUT;
    if (m_connection.state == TcpState::CloseWait ||
        m_connection.state == TcpState::Closed) r |= POLLHUP;
    return r;
  }

  void on_segment(const TcpHeader* hdr, const uint8_t* data, size_t data_len);
  TcpConnection& connection() { return m_connection; }

  void on_tick(uint64_t now_ticks);

  static void register_socket(TcpSocket* s);
  static void unregister_socket(TcpSocket* s);
  static void tick_all(uint64_t now_ticks);

private:
  bool m_so_reuseaddr{false};
  bool m_tcp_nodelay{false};
  bool m_so_keepalive{false};
  int m_so_error{0};

  void process_handshake(const TcpHeader* hdr, uint8_t flags, uint32_t seq);
  void process_ack(const TcpHeader* hdr, uint8_t flags);
  void process_data(const TcpHeader* hdr, uint8_t flags, const uint8_t* data, size_t data_len, uint32_t seq);
  void process_fin(const TcpHeader* hdr, uint8_t flags);
  void arm_retransmit();
  void cancel_retransmit();
  void do_retransmit();
};

}
}
