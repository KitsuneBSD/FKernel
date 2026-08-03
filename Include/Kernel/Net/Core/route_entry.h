#pragma once

#include <Kernel/Net/Ip/ip_address.h>

namespace fkernel {
namespace net {

struct RouteEntry {
    IPv4Address destination;
    IPv4Address netmask;
    IPv4Address gateway;
    bool is_default;
};

} // namespace net
} // namespace fkernel
