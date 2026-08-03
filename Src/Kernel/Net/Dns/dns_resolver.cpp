#include <Kernel/Net/Dns/dns_resolver.h>
#include <Kernel/Net/Dns/dns_packet.h>
#include <Kernel/Net/Sockets/udp_socket.h>
#include <Kernel/Net/Core/network_stack.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {
namespace net {

static bool skip_dns_name(const uint8_t*& p, const uint8_t* end) {
    while (p < end && *p != 0) {
        if ((*p & 0xC0) == 0xC0) { p += 2; return false; }
        p += 1 + *p;
    }
    if (p < end) ++p;
    return true;
}

DnsResolver& DnsResolver::the() {
    static DnsResolver instance(IPv4Address(8, 8, 8, 8));
    return instance;
}

bool dns_parse_response(const uint8_t* buf, size_t len, uint32_t& ip_out) {
    if (len < sizeof(DnsHeader)) return false;
    const auto* hdr = reinterpret_cast<const DnsHeader*>(buf);
    if (!(ntohs(hdr->flags) & DnsHeader::FLAG_QR_RESPONSE)) return false;
    if ((ntohs(hdr->flags) & DnsHeader::RCODE_MASK) != 0) return false;
    uint16_t qdcount = ntohs(hdr->qdcount);
    uint16_t ancount = ntohs(hdr->ancount);
    if (ancount == 0) return false;

    const uint8_t* p = buf + sizeof(DnsHeader);
    const uint8_t* end = buf + len;

    for (uint16_t i = 0; i < qdcount && p < end; ++i) {
        skip_dns_name(p, end);
        p += 4;
    }

    for (uint16_t i = 0; i < ancount && p < end; ++i) {
        skip_dns_name(p, end);
        if (p + 10 > end) return false;
        uint16_t rtype  = (uint16_t)((p[0] << 8) | p[1]);
        uint16_t rclass = (uint16_t)((p[2] << 8) | p[3]);
        uint16_t rdlen  = (uint16_t)((p[8] << 8) | p[9]);
        p += 10;
        if (rtype == DNS_TYPE_A && rclass == DNS_CLASS_IN && rdlen == 4 && p + 4 <= end) {
            ip_out = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
                   | ((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
            return true;
        }
        p += rdlen;
    }
    return false;
}

fk::core::Result<IPv4Address, fk::core::Error> DnsResolver::resolve(const char* hostname) {
    uint8_t pkt[DNS_MAX_PACKET];
    size_t pkt_len = 0;

    auto* hdr = reinterpret_cast<DnsHeader*>(pkt);
    hdr->id      = htons(m_next_id++);
    hdr->flags   = htons(DnsHeader::FLAG_OPCODE_QUERY | DnsHeader::FLAG_RD);
    hdr->qdcount = htons(1);
    hdr->ancount = 0;
    hdr->nscount = 0;
    hdr->arcount = 0;
    pkt_len = sizeof(DnsHeader);

    size_t name_len = dns_encode_name(hostname, pkt + pkt_len,
                                      DNS_MAX_PACKET - pkt_len - 4);
    if (name_len == 0) return fk::core::Error::InvalidParameter;
    pkt_len += name_len;
    pkt[pkt_len++] = 0; pkt[pkt_len++] = (uint8_t)DNS_TYPE_A;
    pkt[pkt_len++] = 0; pkt[pkt_len++] = (uint8_t)DNS_CLASS_IN;

    UdpSocket sock;
    static uint16_t local_port_counter = 10000;
    if (sock.bind_port(local_port_counter++).is_error())
        return fk::core::Error::IOError;
    sock.set_remote(m_server, DNS_PORT);

    auto send_res = sock.write(0, pkt_len, pkt);
    if (send_res.is_error()) {
        fk::algorithms::kwarn("DNS", "send failed for '%s'", hostname);
        NetworkStack::the().unregister_udp_socket(sock.local_port());
        return fk::core::Error::IOError;
    }

    // Wait for DNS reply (5s timeout, yield each iteration)
    uint8_t resp[DNS_MAX_PACKET];
    {
        uint64_t deadline = TickManager::the().get_ticks() +
                            5ULL * TickManager::the().get_frequency();
        while (TickManager::the().get_ticks() < deadline) {
            auto r = sock.read(0, DNS_MAX_PACKET, resp);
            if (!r.is_error() && r.value() > 0) {
                uint32_t ip_host = 0;
                if (dns_parse_response(resp, r.value(), ip_host)) {
                    NetworkStack::the().unregister_udp_socket(sock.local_port());
                    IPv4Address addr(ip_host);
                    fk::algorithms::klog("DNS", "Resolved '%s' -> %u.%u.%u.%u", hostname,
                                         (addr.value >> 24) & 0xFF, (addr.value >> 16) & 0xFF,
                                         (addr.value >>  8) & 0xFF,  addr.value        & 0xFF);
                    return addr;
                }
            }
            SchedulerManager::the().yield();
        }
    }

    NetworkStack::the().unregister_udp_socket(sock.local_port());
    fk::algorithms::kwarn("DNS", "Timeout resolving '%s'", hostname);
    return fk::core::Error::Timeout;
}

} // namespace net
} // namespace fkernel
