#pragma once

#include <Kernel/Net/Ip/ip_address.h>
#include <LibFK/Core/result.h>

namespace fkernel {
namespace net {

// Synchronous DHCP client: DISCOVER → OFFER → REQUEST → ACK.
// Applies the assigned address to NetworkStack on success.
class DhcpClient {
    IPv4Address m_assigned_ip;
    IPv4Address m_subnet_mask;
    IPv4Address m_gateway;
    IPv4Address m_dns_server;
    uint32_t    m_lease_seconds{0};
    bool        m_has_address{false};

public:
    static DhcpClient& the();

    fk::core::Result<void, fk::core::Error> acquire_address();

    bool        has_address()   const { return m_has_address; }
    IPv4Address assigned_ip()   const { return m_assigned_ip; }
    IPv4Address subnet_mask()   const { return m_subnet_mask; }
    IPv4Address gateway()       const { return m_gateway; }
    IPv4Address dns_server()    const { return m_dns_server; }
    uint32_t    lease_seconds() const { return m_lease_seconds; }
};

} // namespace net
} // namespace fkernel
