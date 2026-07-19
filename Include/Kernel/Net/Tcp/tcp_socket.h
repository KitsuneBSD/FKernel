#pragma once

#include <Kernel/Net/Ip/ip_address.h>
#include <Kernel/Net/Tcp/tcp_connection.h>
#include <Kernel/Net/socket.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace net {

class TcpSocket : public Socket {
  TcpConnection m_connection;
  fk::synchronization::Spinlock m_lock;

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
  size_t size() const override { return 0; }
  bool is_directory() const override { return false; }

  void on_segment(const TcpHeader* hdr, const uint8_t* data, size_t data_len);
  TcpConnection& connection() { return m_connection; }

private:
  void process_handshake(const TcpHeader* hdr, uint8_t flags, uint32_t seq);
  void process_ack(const TcpHeader* hdr, uint8_t flags);
  void process_data(const TcpHeader* hdr, uint8_t flags, const uint8_t* data, size_t data_len, uint32_t seq);
  void process_fin(const TcpHeader* hdr, uint8_t flags);
};

} // namespace net
} // namespace fkernel
