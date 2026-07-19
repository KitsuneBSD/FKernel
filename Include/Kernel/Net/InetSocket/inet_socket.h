#pragma once

#include <Kernel/Net/Ip/ip_address.h>
#include <Kernel/Net/socket.h>
#include <LibFK/Core/Result.h>

namespace fkernel {
namespace net {

fk::core::Result<fk::RefPtr<Socket>, fk::core::Error>
create_inet_socket(SocketType type);

} // namespace net
} // namespace fkernel
