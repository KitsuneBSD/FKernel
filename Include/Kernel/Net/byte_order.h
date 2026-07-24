#pragma once

// Canonical definition in LibFK; import into fkernel::net for compatibility.
#include <LibFK/Algorithms/byte_order.h>

namespace fkernel {
namespace net {
using fk::algorithms::htons;
using fk::algorithms::ntohs;
using fk::algorithms::htonl;
using fk::algorithms::ntohl;
} // namespace net
} // namespace fkernel
