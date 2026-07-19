#include <Kernel/Net/NetworkStack/network_stack.h>
#include <Kernel/Net/Arp/arp_packet.h>
#include <Kernel/Net/Arp/arp_table.h>
#include <Kernel/Net/Eth/ethernet_frame.h>
#include <Kernel/Net/Icmp/icmp_packet.h>
#include <Kernel/Net/Ip/ipv4_header.h>
#include <Kernel/Net/Routing/routing_table.h>
#include <Kernel/Net/Tcp/tcp_header.h>
#include <Kernel/Net/Udp/udp_header.h>
#include <Kernel/Net/Tcp/tcp_socket.h>
#include <Kernel/Net/Udp/udp_socket.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {
namespace net {

NetworkStack::NetworkStack() : m_device(nullptr), m_ip(IPv4Address::any()) {}

NetworkStack& NetworkStack::the() {
    static NetworkStack instance;
    return instance;
}

void NetworkStack::set_device(NetworkDevice* dev) { m_device = dev; }
void NetworkStack::set_ip(IPv4Address ip) { m_ip = ip; }
void NetworkStack::set_gateway(IPv4Address gw) { RoutingTable::the().set_default_gateway(gw); }

static constexpr size_t MAX_FRAME_SIZE = 1514;

fk::core::Result<void, fk::core::Error> NetworkStack::send_packet(
    const MACAddress& dst, uint16_t ethertype,
    const uint8_t* payload, size_t payload_len) {
    if (!m_device) return fk::core::Error::DeviceError;
    if (ETHERNET_HEADER_SIZE + payload_len > MAX_FRAME_SIZE)
        return fk::core::Error::InvalidParameter;
    uint8_t frame[MAX_FRAME_SIZE];
    auto* eth = reinterpret_cast<EthernetHeader*>(frame);
    eth->set_dst(dst);
    eth->set_src(m_device->mac_address());
    eth->ethertype = htons(ethertype);
    memcpy(frame + ETHERNET_HEADER_SIZE, payload, payload_len);
    return m_device->send_packet(frame, ETHERNET_HEADER_SIZE + payload_len);
}

fk::core::Result<void, fk::core::Error> NetworkStack::send_arp_request(IPv4Address target_ip) {
    if (!m_device) return fk::core::Error::DeviceError;
    ArpPacket arp;
    arp.hw_type    = htons(ARP_HW_ETHERNET);
    arp.proto_type = htons(ARP_PROTO_IPV4);
    arp.hw_len     = 6;
    arp.proto_len  = 4;
    arp.operation  = htons(ARP_OP_REQUEST);
    auto my_mac = m_device->mac_address();
    for (int i = 0; i < 6; ++i) { arp.sender_mac[i] = my_mac.address[i]; arp.target_mac[i] = 0xFF; }
    arp.sender_ip = m_ip.to_network();
    arp.target_ip = target_ip.to_network();
    MACAddress broadcast(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    return send_packet(broadcast, ETHERTYPE_ARP,
                       reinterpret_cast<const uint8_t*>(&arp), ARP_PACKET_SIZE);
}

fk::core::Result<void, fk::core::Error> NetworkStack::send_ipv4(
    IPv4Address dst_ip, uint8_t proto,
    const uint8_t* payload, size_t payload_len) {
    auto next_hop = RoutingTable::the().lookup(dst_ip);
    IPv4Address arp_target = next_hop.has_value() ? next_hop.value() : dst_ip;
    auto mac_opt = ArpTable::the().lookup(arp_target);
    if (!mac_opt.has_value()) {
        send_arp_request(arp_target);
        return fk::core::Error::NotFound;
    }
    static constexpr size_t MAX_IP_SIZE = MAX_FRAME_SIZE - ETHERNET_HEADER_SIZE;
    size_t ip_size = IPV4_HEADER_SIZE + payload_len;
    if (ip_size > MAX_IP_SIZE) return fk::core::Error::InvalidParameter;
    uint8_t ip_packet[MAX_IP_SIZE];
    auto* iph = reinterpret_cast<IPv4Header*>(ip_packet);
    iph->fill(proto, m_ip.to_network(), dst_ip.to_network(), (uint16_t)payload_len);
    memcpy(ip_packet + IPV4_HEADER_SIZE, payload, payload_len);
    return send_packet(mac_opt.value(), ETHERTYPE_IPV4, ip_packet, ip_size);
}

void NetworkStack::on_packet(const uint8_t* data, size_t len) {
    if (len < ETHERNET_HEADER_SIZE) return;
    const auto* eth = reinterpret_cast<const EthernetHeader*>(data);
    uint16_t et = ntohs(eth->ethertype);
    const uint8_t* payload = data + ETHERNET_HEADER_SIZE;
    size_t payload_len = len - ETHERNET_HEADER_SIZE;
    if (et == ETHERTYPE_ARP)  { handle_arp(payload, payload_len); return; }
    if (et == ETHERTYPE_IPV4) { handle_ipv4(payload, payload_len); }
}

void NetworkStack::handle_arp(const uint8_t* data, size_t len) {
    if (len < ARP_PACKET_SIZE) return;
    const auto* arp = reinterpret_cast<const ArpPacket*>(data);
    IPv4Address sender_ip = IPv4Address::from_network(arp->sender_ip);
    MACAddress  sender_mac(arp->sender_mac[0], arp->sender_mac[1], arp->sender_mac[2],
                           arp->sender_mac[3], arp->sender_mac[4], arp->sender_mac[5]);
    ArpTable::the().update(sender_ip, sender_mac);
    if (ntohs(arp->operation) != ARP_OP_REQUEST) return;
    if (IPv4Address::from_network(arp->target_ip) != m_ip) return;
    ArpPacket reply;
    reply.hw_type    = htons(ARP_HW_ETHERNET);
    reply.proto_type = htons(ARP_PROTO_IPV4);
    reply.hw_len     = 6;
    reply.proto_len  = 4;
    reply.operation  = htons(ARP_OP_REPLY);
    auto my_mac = m_device->mac_address();
    for (int i = 0; i < 6; ++i) reply.sender_mac[i] = my_mac.address[i];
    reply.sender_ip = m_ip.to_network();
    for (int i = 0; i < 6; ++i) reply.target_mac[i] = arp->sender_mac[i];
    reply.target_ip = arp->sender_ip;
    send_packet(sender_mac, ETHERTYPE_ARP,
                reinterpret_cast<const uint8_t*>(&reply), ARP_PACKET_SIZE);
}

void NetworkStack::handle_ipv4(const uint8_t* data, size_t len) {
    if (len < IPV4_HEADER_SIZE) return;
    const auto* iph = reinterpret_cast<const IPv4Header*>(data);
    if (iph->version() != 4) return;
    uint8_t ihl = iph->ihl();
    if (len < ihl) return;
    const uint8_t* payload = data + ihl;
    size_t payload_len = len - ihl;
    if (iph->protocol == IP_PROTO_ICMP) { handle_icmp(payload, payload_len, iph->src_ip); }
    else if (iph->protocol == IP_PROTO_UDP) { handle_udp(payload, payload_len, iph->src_ip, iph->dst_ip); }
    else if (iph->protocol == IP_PROTO_TCP) { handle_tcp(payload, payload_len, iph->src_ip, iph->dst_ip); }
}

void NetworkStack::handle_icmp(const uint8_t* data, size_t len, uint32_t src_ip) {
    if (len < ICMP_HEADER_SIZE) return;
    const auto* icmp = reinterpret_cast<const IcmpHeader*>(data);
    if (icmp->type != ICMP_TYPE_ECHO_REQUEST) return;
    static constexpr size_t MAX_ICMP_SIZE = 1480;
    size_t payload_len = len - ICMP_HEADER_SIZE;
    size_t reply_size = ICMP_HEADER_SIZE + payload_len;
    if (reply_size > MAX_ICMP_SIZE) return;
    uint8_t reply_buf[MAX_ICMP_SIZE];
    auto* reply = reinterpret_cast<IcmpHeader*>(reply_buf);
    reply->type       = ICMP_TYPE_ECHO_REPLY;
    reply->code       = 0;
    reply->checksum   = 0;
    reply->identifier = icmp->identifier;
    reply->sequence   = icmp->sequence;
    memcpy(reply_buf + ICMP_HEADER_SIZE, data + ICMP_HEADER_SIZE, payload_len);
    reply->checksum = reply->compute_checksum(reply_buf + ICMP_HEADER_SIZE, payload_len);
    send_ipv4(IPv4Address::from_network(src_ip), IP_PROTO_ICMP, reply_buf, reply_size);
}

void NetworkStack::handle_udp(const uint8_t* data, size_t len,
                               uint32_t src_ip, [[maybe_unused]] uint32_t dst_ip) {
    if (len < UDP_HEADER_SIZE) return;
    const auto* hdr = reinterpret_cast<const UdpHeader*>(data);
    uint16_t dst_port = ntohs(hdr->dst_port);
    uint16_t src_port = ntohs(hdr->src_port);
    const uint8_t* payload = data + UDP_HEADER_SIZE;
    size_t payload_len = len - UDP_HEADER_SIZE;

    fk::synchronization::ScopedLock lock(m_udp_lock);
    if (dst_port < MAX_UDP_SOCKETS && m_udp_sockets[dst_port]) {
        m_udp_sockets[dst_port]->on_receive(
            IPv4Address::from_network(src_ip), src_port, payload, payload_len);
    }
}

bool NetworkStack::register_udp_socket(uint16_t port, UdpSocket* socket) {
    fk::synchronization::ScopedLock lock(m_udp_lock);
    if (port >= MAX_UDP_SOCKETS || m_udp_sockets[port]) return false;
    m_udp_sockets[port] = socket;
    return true;
}

void NetworkStack::unregister_udp_socket(uint16_t port) {
    fk::synchronization::ScopedLock lock(m_udp_lock);
    if (port < MAX_UDP_SOCKETS) m_udp_sockets[port] = nullptr;
}

void NetworkStack::handle_tcp(const uint8_t* data, size_t len,
                               [[maybe_unused]] uint32_t src_ip, [[maybe_unused]] uint32_t dst_ip) {
    if (len < TCP_HEADER_SIZE) return;
    const auto* hdr = reinterpret_cast<const TcpHeader*>(data);
    uint16_t dst_port = ntohs(hdr->dst_port);
    uint8_t header_len = hdr->header_length();
    if (len < header_len) return;
    const uint8_t* payload = data + header_len;
    size_t payload_len = len - header_len;

    fk::synchronization::ScopedLock lock(m_tcp_lock);
    if (dst_port < MAX_TCP_SOCKETS && m_tcp_sockets[dst_port])
        m_tcp_sockets[dst_port]->on_segment(hdr, payload, payload_len);
}

bool NetworkStack::register_tcp_socket(uint16_t port, TcpSocket* socket) {
    fk::synchronization::ScopedLock lock(m_tcp_lock);
    if (port >= MAX_TCP_SOCKETS || m_tcp_sockets[port]) return false;
    m_tcp_sockets[port] = socket;
    return true;
}

void NetworkStack::unregister_tcp_socket(uint16_t port) {
    fk::synchronization::ScopedLock lock(m_tcp_lock);
    if (port < MAX_TCP_SOCKETS) m_tcp_sockets[port] = nullptr;
}

} // namespace net
} // namespace fkernel
