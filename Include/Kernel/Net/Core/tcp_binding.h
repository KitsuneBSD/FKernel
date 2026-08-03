#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace net {

class TcpSocket;

struct TcpBinding { uint16_t port{0}; TcpSocket* socket{nullptr}; };

} // namespace net
} // namespace fkernel
