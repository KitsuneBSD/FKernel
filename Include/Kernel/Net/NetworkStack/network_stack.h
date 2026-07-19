#pragma once

#include <Kernel/Driver/Network/network_device.h>
#include <Kernel/Net/Eth/ethernet_frame.h>
#include <Kernel/Net/Ip/ip_address.h>
#include <LibC/stdint.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace net {

class UdpSocket;
class TcpSocket;

static constexpr size_t MAX_UDP_SOCKETS = 64;
static constexpr size_t MAX_TCP_SOCKETS = 64;

class NetworkStack {
  NetworkDevice* m_device;
  IPv4Address    m_ip;

  UdpSocket*    m_udp_sockets[MAX_UDP_SOCKETS]{};
  fk::synchronization::Spinlock m_udp_lock;

  TcpSocket*    m_tcp_sockets[MAX_TCP_SOCKETS]{};
  fk::synchronization::Spinlock m_tcp_lock;

public:
  static NetworkStack& the();

  void set_device(NetworkDevice* dev);
  void set_ip(IPv4Address ip);
  void set_gateway(IPv4Address gw);
  NetworkDevice* device() const { return m_device; }
  IPv4Address ip() const { return m_ip; }

  void on_packet(const uint8_t* data, size_t len);

  fk::core::Result<void, fk::core::Error> send_packet(
      const MACAddress& dst, uint16_t ethertype,
      const uint8_t* payload, size_t payload_len);

  fk::core::Result<void, fk::core::Error> send_ipv4(
      IPv4Address dst_ip, uint8_t proto,
      const uint8_t* payload, size_t payload_len);

  fk::core::Result<void, fk::core::Error> send_arp_request(IPv4Address target_ip);

  bool register_udp_socket(uint16_t port, UdpSocket* socket);
  void unregister_udp_socket(uint16_t port);

  bool register_tcp_socket(uint16_t port, TcpSocket* socket);
  void unregister_tcp_socket(uint16_t port);

private:
  NetworkStack();

  void handle_arp(const uint8_t* data, size_t len);
  void handle_ipv4(const uint8_t* data, size_t len);
  void handle_icmp(const uint8_t* ip_payload, size_t len, uint32_t src_ip);
  void handle_udp(const uint8_t* ip_payload, size_t len,
                  uint32_t src_ip, uint32_t dst_ip);
  void handle_tcp(const uint8_t* ip_payload, size_t len,
                  uint32_t src_ip, uint32_t dst_ip);
};

} // namespace net
} // namespace fkernel
