#pragma once

#include <LibC/stdint.h>
#include <Kernel/Net/byte_order.h>

namespace fkernel {
namespace net {

static constexpr uint16_t ARP_HW_ETHERNET = 1;
static constexpr uint16_t ARP_PROTO_IPV4  = 0x0800;
static constexpr uint16_t ARP_OP_REQUEST  = 1;
static constexpr uint16_t ARP_OP_REPLY    = 2;

struct __attribute__((packed)) ArpPacket {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t  hw_len;
    uint8_t  proto_len;
    uint16_t operation;
    uint8_t  sender_mac[6];
    uint32_t sender_ip;
    uint8_t  target_mac[6];
    uint32_t target_ip;
};

static constexpr size_t ARP_PACKET_SIZE = 28;

} // namespace net
} // namespace fkernel
