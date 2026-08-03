#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace net {

class UdpSocket;

struct UdpBinding { uint16_t port{0}; UdpSocket* socket{nullptr}; };

} // namespace net
} // namespace fkernel
