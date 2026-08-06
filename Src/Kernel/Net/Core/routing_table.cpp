#include <Kernel/Net/Core/routing_table.h>

namespace fkernel {
namespace net {

RoutingTable& RoutingTable::the() {
    static RoutingTable instance;
    return instance;
}

bool RoutingTable::matches(IPv4Address dest, IPv4Address net, IPv4Address mask) {
    return (dest.value & mask.value) == (net.value & mask.value);
}

void RoutingTable::set_default_gateway(IPv4Address gw) {
    m_default_gateway = fk::memory::optional<IPv4Address>(gw);
}

void RoutingTable::add_route(IPv4Address dest, IPv4Address mask, IPv4Address gw) {
    RouteEntry entry;
    entry.destination = dest;
    entry.netmask = mask;
    entry.gateway = gw;
    entry.is_default = false;
    TRY_OR_FATAL(m_routes.push_back(entry));
}

fk::memory::optional<IPv4Address> RoutingTable::lookup(IPv4Address dest) const {
    for (size_t i = 0; i < m_routes.size(); ++i) {
        if (matches(dest, m_routes[i].destination, m_routes[i].netmask))
            return fk::memory::optional<IPv4Address>(m_routes[i].gateway);
    }
    if (m_default_gateway.has_value())
        return m_default_gateway;
    return fk::memory::optional<IPv4Address>();
}

} // namespace net
} // namespace fkernel
