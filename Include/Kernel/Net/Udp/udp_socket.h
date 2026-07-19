#pragma once

#include <Kernel/Net/Ip/ip_address.h>
#include <Kernel/Net/socket.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace net {

class UdpSocket : public Socket {
  uint16_t m_local_port;
  IPv4Address m_remote_ip;

public:
  UdpSocket();

  SocketDomain domain() const override { return SocketDomain::Inet; }
  SocketType   type()   const override { return SocketType::Datagram; }

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

  void on_receive(IPv4Address src, uint16_t src_port,
                  const uint8_t* data, size_t len);

  uint16_t local_port() const { return m_local_port; }

private:
  uint16_t           m_remote_port{0};
  fk::containers::Vector<uint8_t> m_recv_buf;
  fk::synchronization::Spinlock   m_lock;
};

} // namespace net
} // namespace fkernel
