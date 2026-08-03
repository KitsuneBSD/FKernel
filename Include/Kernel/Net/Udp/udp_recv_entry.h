#pragma once

#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace net {

struct UdpRecvEntry {
    uint32_t src_ip{0};
    uint16_t src_port{0};
    fk::containers::Vector<uint8_t> data;
};

} // namespace net
} // namespace fkernel
