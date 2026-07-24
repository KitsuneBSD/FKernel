#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Types/types.h>

namespace fkernel {
namespace net {

static constexpr uint8_t ICMP_TYPE_ECHO_REPLY   = 0;
static constexpr uint8_t ICMP_TYPE_ECHO_REQUEST  = 8;

struct __attribute__((packed)) IcmpHeader {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;

    uint16_t compute_checksum(const uint8_t* payload, size_t payload_len) const;
};

static constexpr size_t ICMP_HEADER_SIZE = 8;

} // namespace net
} // namespace fkernel
