#pragma once

#include <Kernel/Net/Dns/dns_packet.h>
#include <Kernel/Net/Ip/ip_address.h>
#include <LibFK/Core/result.h>
#include <LibFK/Memory/optional.h>

namespace fkernel {
namespace net {

// Synchronous, single-shot DNS A-record resolver.
// Sends a UDP query to `server` on port 53 and waits for a response.
// Returns the first resolved IPv4 address or an error.
class DnsResolver {
    IPv4Address m_server;
    uint16_t m_next_id{1};

public:
    explicit DnsResolver(IPv4Address server) : m_server(server) {}

    // Resolve a hostname to an IPv4 address.
    fk::core::Result<IPv4Address, fk::core::Error> resolve(const char* hostname);

    void set_server(IPv4Address server) { m_server = server; }
    IPv4Address server() const { return m_server; }

    static DnsResolver& the();
};

} // namespace net
} // namespace fkernel
