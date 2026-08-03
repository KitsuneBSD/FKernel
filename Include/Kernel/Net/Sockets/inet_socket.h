#pragma once

#include <Kernel/Net/Ip/ip_address.h>
#include <Kernel/Net/Sockets/socket.h>
#include <LibFK/Core/result.h>

namespace fkernel {
namespace net {

fk::core::Result<fk::RefPtr<Socket>, fk::core::Error>
create_inet_socket(SocketType type);

} // namespace net
} // namespace fkernel
