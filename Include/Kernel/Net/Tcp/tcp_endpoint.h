#pragma once

#include <Kernel/Net/Ip/ip_address.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace net {

struct TcpEndpoint {
  IPv4Address ip;
  uint16_t    port;
};

} // namespace net
} // namespace fkernel
