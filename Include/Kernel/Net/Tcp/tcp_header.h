#pragma once

#include <LibC/stdint.h>
#include <Kernel/Net/byte_order.h>

namespace fkernel {
namespace net {

static constexpr uint8_t TCP_FLAG_FIN = 0x01;
static constexpr uint8_t TCP_FLAG_SYN = 0x02;
static constexpr uint8_t TCP_FLAG_RST = 0x04;
static constexpr uint8_t TCP_FLAG_PSH = 0x08;
static constexpr uint8_t TCP_FLAG_ACK = 0x10;
static constexpr uint8_t TCP_FLAG_URG = 0x20;

struct __attribute__((packed)) TcpHeader {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t  data_offset; // top 4 bits = header len in 32-bit words
    uint8_t  flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;

    uint8_t header_length() const { return (data_offset >> 4) * 4; }

    void fill(uint16_t src, uint16_t dst, uint32_t seq, uint32_t ack,
              uint8_t fl, uint16_t win) {
        src_port   = htons(src);
        dst_port   = htons(dst);
        seq_num    = htonl(seq);
        ack_num    = htonl(ack);
        data_offset = 0x50; // 5 * 4 = 20 bytes header, no options
        flags      = fl;
        window     = htons(win);
        checksum   = 0;
        urgent_ptr = 0;
    }
};

static constexpr size_t TCP_HEADER_SIZE = 20;

} // namespace net
} // namespace fkernel
