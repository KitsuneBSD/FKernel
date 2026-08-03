#pragma once

#include <Kernel/Net/Ip/ip_address.h>
#include <Kernel/Net/Core/route_entry.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Memory/optional.h>

namespace fkernel {
namespace net {

class RoutingTable {
    fk::containers::Vector<RouteEntry> m_routes;
    fk::memory::optional<IPv4Address> m_default_gateway;

public:
    static RoutingTable& the();

    void set_default_gateway(IPv4Address gw);
    void add_route(IPv4Address dest, IPv4Address mask, IPv4Address gw);

    fk::memory::optional<IPv4Address> lookup(IPv4Address dest) const;
    bool has_default_gateway() const { return m_default_gateway.has_value(); }
    IPv4Address default_gateway() const { return m_default_gateway.value(); }

private:
    static bool matches(IPv4Address dest, IPv4Address net, IPv4Address mask);
};

} // namespace net
} // namespace fkernel
