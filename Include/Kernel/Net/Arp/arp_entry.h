#pragma once

#include <Kernel/Driver/Network/mac_address.h>
#include <Kernel/Net/Ip/ip_address.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace net {

struct ArpEntry {
    MACAddress  mac;
    uint64_t    created_at_ticks{0};
};

} // namespace net
} // namespace fkernel
