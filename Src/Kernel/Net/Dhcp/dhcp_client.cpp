#include <Kernel/Net/Dhcp/dhcp_client.h>
#include <Kernel/Net/Dhcp/dhcp_packet.h>
#include <Kernel/Net/Dns/dns_resolver.h>
#include <Kernel/Net/Udp/udp_socket.h>
#include <Kernel/Net/NetworkStack/network_stack.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {
namespace net {

DhcpClient& DhcpClient::the() {
    static DhcpClient instance;
    return instance;
}

static size_t append_option(uint8_t* opts, size_t pos, uint8_t code, const uint8_t* data, uint8_t len) {
    opts[pos++] = code;
    opts[pos++] = len;
    for (uint8_t i = 0; i < len; ++i) opts[pos++] = data[i];
    return pos;
}

static size_t build_discover(uint8_t* pkt, uint32_t xid, const uint8_t mac[6]) {
    fk::memory::set(pkt, 0, DHCP_MAX_PACKET);
    auto* hdr = reinterpret_cast<DhcpHeader*>(pkt);
    hdr->op    = DHCP_OP_REQUEST;
    hdr->htype = DHCP_HTYPE_ETH;
    hdr->hlen  = DHCP_HLEN_ETH;
    hdr->xid   = htonl(xid);
    hdr->flags = htons(DHCP_FLAG_BROADCAST);
    for (int i = 0; i < 6; ++i) hdr->chaddr[i] = mac[i];
    for (int i = 0; i < 4; ++i) hdr->magic[i] = DHCP_MAGIC[i];

    uint8_t* opts = pkt + DHCP_HEADER_SIZE;
    size_t pos = 0;
    uint8_t msg_type = DHCPDISCOVER;
    pos = append_option(opts, pos, DHCP_OPT_MSGTYPE, &msg_type, 1);
    opts[pos++] = DHCP_OPT_END;
    return DHCP_HEADER_SIZE + pos;
}

static size_t build_request(uint8_t* pkt, uint32_t xid, const uint8_t mac[6],
                             uint32_t requested_ip, uint32_t server_ip) {
    fk::memory::set(pkt, 0, DHCP_MAX_PACKET);
    auto* hdr = reinterpret_cast<DhcpHeader*>(pkt);
    hdr->op    = DHCP_OP_REQUEST;
    hdr->htype = DHCP_HTYPE_ETH;
    hdr->hlen  = DHCP_HLEN_ETH;
    hdr->xid   = htonl(xid);
    hdr->flags = htons(DHCP_FLAG_BROADCAST);
    for (int i = 0; i < 6; ++i) hdr->chaddr[i] = mac[i];
    for (int i = 0; i < 4; ++i) hdr->magic[i] = DHCP_MAGIC[i];

    uint8_t* opts = pkt + DHCP_HEADER_SIZE;
    size_t pos = 0;
    uint8_t msg_type = DHCPREQUEST;
    pos = append_option(opts, pos, DHCP_OPT_MSGTYPE, &msg_type, 1);
    uint8_t req_ip[4] = {
        (uint8_t)(requested_ip >> 24), (uint8_t)(requested_ip >> 16),
        (uint8_t)(requested_ip >>  8), (uint8_t)(requested_ip)
    };
    pos = append_option(opts, pos, DHCP_OPT_REQ_IP, req_ip, 4);
    uint8_t srv_ip[4] = {
        (uint8_t)(server_ip >> 24), (uint8_t)(server_ip >> 16),
        (uint8_t)(server_ip >>  8), (uint8_t)(server_ip)
    };
    pos = append_option(opts, pos, DHCP_OPT_SERVER_ID, srv_ip, 4);
    opts[pos++] = DHCP_OPT_END;
    return DHCP_HEADER_SIZE + pos;
}

struct DhcpOffer {
    uint32_t yiaddr{0};
    uint32_t server_id{0};
    uint32_t subnet_mask{0};
    uint32_t gateway{0};
    uint32_t dns_server{0};
    uint32_t lease_seconds{0};
    uint8_t  msg_type{0};
};

static bool parse_response(const uint8_t* pkt, size_t len, uint32_t xid, DhcpOffer& out) {
    if (len < DHCP_HEADER_SIZE) return false;
    const auto* hdr = reinterpret_cast<const DhcpHeader*>(pkt);
    if (hdr->op != DHCP_OP_REPLY) return false;
    if (ntohl(hdr->xid) != xid) return false;
    for (int i = 0; i < 4; ++i) {
        if (hdr->magic[i] != DHCP_MAGIC[i]) return false;
    }
    out.yiaddr = ntohl(hdr->yiaddr);

    const uint8_t* opts = pkt + DHCP_HEADER_SIZE;
    const uint8_t* end  = pkt + len;
    const uint8_t* p = opts;
    while (p < end) {
        uint8_t code = *p++;
        if (code == DHCP_OPT_END) break;
        if (code == 0) continue; // pad
        if (p >= end) break;
        uint8_t optlen = *p++;
        if (p + optlen > end) break;
        switch (code) {
        case DHCP_OPT_MSGTYPE:
            if (optlen >= 1) out.msg_type = p[0];
            break;
        case DHCP_OPT_SERVER_ID:
            if (optlen >= 4)
                out.server_id = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
                              | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
            break;
        case DHCP_OPT_SUBNET:
            if (optlen >= 4)
                out.subnet_mask = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
                                | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
            break;
        case DHCP_OPT_ROUTER:
            if (optlen >= 4)
                out.gateway = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
                            | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
            break;
        case DHCP_OPT_DNS:
            if (optlen >= 4)
                out.dns_server = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
                               | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
            break;
        case DHCP_OPT_LEASE:
            if (optlen >= 4)
                out.lease_seconds = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
                                  | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
            break;
        }
        p += optlen;
    }
    return out.msg_type != 0;
}

fk::core::Result<void, fk::core::Error> DhcpClient::acquire_address() {
    auto* dev = NetworkStack::the().device();
    if (!dev) return fk::core::Error::DeviceError;
    auto mac = dev->mac_address();
    uint32_t xid = (uint32_t)TickManager::the().get_ticks() ^ 0xDEADBEEFu;

    UdpSocket sock;
    if (sock.bind_port(DHCP_CLIENT_PORT).is_error())
        return fk::core::Error::AlreadyExists;
    sock.set_remote(IPv4Address::broadcast(), DHCP_SERVER_PORT);

    uint8_t pkt[DHCP_MAX_PACKET];
    uint8_t resp[DHCP_MAX_PACKET];

    // Phase 1: DISCOVER
    size_t len = build_discover(pkt, xid, mac.address);
    if (sock.write(0, len, pkt).is_error()) {
        NetworkStack::the().unregister_udp_socket(DHCP_CLIENT_PORT);
        return fk::core::Error::IOError;
    }
    fk::algorithms::klog("DHCP", "DISCOVER sent");

    // Wait for OFFER (5s timeout, yield each iteration)
    DhcpOffer offer;
    bool got_offer = false;
    {
        uint64_t deadline = TickManager::the().get_ticks() +
                            5ULL * TickManager::the().get_frequency();
        while (!got_offer && TickManager::the().get_ticks() < deadline) {
            auto r = sock.read(0, DHCP_MAX_PACKET, resp);
            if (!r.is_error() && r.value() >= DHCP_HEADER_SIZE) {
                DhcpOffer parsed;
                if (parse_response(resp, r.value(), xid, parsed) &&
                    parsed.msg_type == DHCPOFFER && parsed.yiaddr != 0) {
                    offer = parsed;
                    got_offer = true;
                }
            }
            if (!got_offer) SchedulerManager::the().yield();
        }
    }
    if (!got_offer) {
        NetworkStack::the().unregister_udp_socket(DHCP_CLIENT_PORT);
        fk::algorithms::kwarn("DHCP", "No OFFER received");
        return fk::core::Error::Timeout;
    }
    fk::algorithms::klog("DHCP", "OFFER received: %u.%u.%u.%u",
        (offer.yiaddr >> 24) & 0xFF, (offer.yiaddr >> 16) & 0xFF,
        (offer.yiaddr >> 8)  & 0xFF,  offer.yiaddr & 0xFF);

    // Phase 2: REQUEST
    len = build_request(pkt, xid, mac.address, offer.yiaddr, offer.server_id);
    if (sock.write(0, len, pkt).is_error()) {
        NetworkStack::the().unregister_udp_socket(DHCP_CLIENT_PORT);
        return fk::core::Error::IOError;
    }
    fk::algorithms::klog("DHCP", "REQUEST sent");

    // Wait for ACK or NAK (5s timeout, yield each iteration)
    bool got_ack = false;
    {
        uint64_t deadline = TickManager::the().get_ticks() +
                            5ULL * TickManager::the().get_frequency();
        while (!got_ack && TickManager::the().get_ticks() < deadline) {
            auto r = sock.read(0, DHCP_MAX_PACKET, resp);
            if (!r.is_error() && r.value() >= DHCP_HEADER_SIZE) {
                DhcpOffer parsed;
                if (parse_response(resp, r.value(), xid, parsed)) {
                    if (parsed.msg_type == DHCPNAK) {
                        NetworkStack::the().unregister_udp_socket(DHCP_CLIENT_PORT);
                        fk::algorithms::kwarn("DHCP", "NAK received");
                        return fk::core::Error::PermissionDenied;
                    }
                    if (parsed.msg_type == DHCPACK && parsed.yiaddr != 0) {
                        offer = parsed;
                        got_ack = true;
                    }
                }
            }
            if (!got_ack) SchedulerManager::the().yield();
        }
    }
    NetworkStack::the().unregister_udp_socket(DHCP_CLIENT_PORT);
    if (!got_ack) {
        fk::algorithms::kwarn("DHCP", "No ACK received");
        return fk::core::Error::Timeout;
    }

    m_assigned_ip    = IPv4Address(offer.yiaddr);
    m_subnet_mask    = IPv4Address(offer.subnet_mask);
    m_gateway        = IPv4Address(offer.gateway);
    m_dns_server     = IPv4Address(offer.dns_server);
    m_lease_seconds  = offer.lease_seconds;
    m_has_address    = true;

    NetworkStack::the().set_ip(m_assigned_ip);
    if (m_gateway.value != 0) NetworkStack::the().set_gateway(m_gateway);
    if (m_dns_server.value != 0) DnsResolver::the().set_server(m_dns_server);

    fk::algorithms::klog("DHCP", "ACK: IP=%u.%u.%u.%u lease=%us",
        (offer.yiaddr >> 24) & 0xFF, (offer.yiaddr >> 16) & 0xFF,
        (offer.yiaddr >> 8)  & 0xFF,  offer.yiaddr & 0xFF,
        offer.lease_seconds);
    return {};
}

} // namespace net
} // namespace fkernel
