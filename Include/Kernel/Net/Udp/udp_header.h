#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Net/byte_order.h>

namespace fkernel {
namespace net {

struct __attribute__((packed)) UdpHeader {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;   // header + data, big-endian
    uint16_t checksum; // 0 = disabled

    void fill(uint16_t src, uint16_t dst, uint16_t data_len) {
        src_port = htons(src);
        dst_port = htons(dst);
        length   = htons((uint16_t)(8 + data_len));
        checksum = 0;
    }
};

static constexpr size_t UDP_HEADER_SIZE = 8;

} // namespace net
} // namespace fkernel
