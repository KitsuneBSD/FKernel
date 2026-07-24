#pragma once

#include <Kernel/Net/Ip/ip_address.h>
#include <Kernel/Net/socket.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace net {

struct UdpRecvEntry {
    uint32_t src_ip{0};
    uint16_t src_port{0};
    fk::containers::Vector<uint8_t> data;
};

class UdpSocket : public Socket {
  uint16_t    m_local_port;
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
  short poll() const override {
    short r = POLLOUT;
    if (!m_recv_queue.is_empty()) r |= POLLIN;
    return r;
  }

  fk::core::Result<size_t, fk::core::Error> sendto(
      size_t size, const uint8_t* buf, const char* addr, uint32_t addrlen) override;
  fk::core::Result<size_t, fk::core::Error> recvfrom(
      size_t size, uint8_t* buf, char* addr, uint32_t* addrlen) override;
  fk::core::Result<void, fk::core::Error> getsockname(
      char* addr, uint32_t* addrlen) override;
  fk::core::Result<void, fk::core::Error> getpeername(
      char* addr, uint32_t* addrlen) override;
  fk::core::Result<void, fk::core::Error> setsockopt(
      int level, int optname, const void* optval, uint32_t optlen) override;
  fk::core::Result<void, fk::core::Error> getsockopt(
      int level, int optname, void* optval, uint32_t* optlen) override;

  void on_receive(IPv4Address src, uint16_t src_port,
                  const uint8_t* data, size_t len);

  uint16_t local_port() const { return m_local_port; }

  fk::core::Result<void, fk::core::Error> bind_port(uint16_t port);
  void set_remote(IPv4Address ip, uint16_t port) {
      m_remote_ip   = ip;
      m_remote_port = port;
  }

private:
  uint16_t m_remote_port{0};
  bool m_so_reuseaddr{false};
  fk::containers::Vector<UdpRecvEntry> m_recv_queue;
  fk::synchronization::Spinlock m_lock;
};

} // namespace net
} // namespace fkernel
