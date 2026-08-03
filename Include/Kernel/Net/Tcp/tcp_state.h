#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace net {

enum class TcpState : uint8_t {
  Closed, Listen, SynSent, SynReceived,
  Established, FinWait1, FinWait2,
  CloseWait, LastAck, TimeWait, Closing,
};

} // namespace net
} // namespace fkernel
