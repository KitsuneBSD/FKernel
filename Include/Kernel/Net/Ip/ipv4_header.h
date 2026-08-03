#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Net/Core/byte_order.h>
#include <LibFK/Algorithms/Crypto/internet_checksum.h>

namespace fkernel {
namespace net {

static constexpr uint8_t IP_PROTO_ICMP = 1;
static constexpr uint8_t IP_PROTO_TCP  = 6;
static constexpr uint8_t IP_PROTO_UDP  = 17;

struct __attribute__((packed)) IPv4Header {
    uint8_t  version_ihl;
    uint8_t  dscp_ecn;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;

    uint8_t version() const { return (version_ihl >> 4) & 0xF; }
    uint8_t ihl()     const { return (version_ihl & 0xF) * 4; }

    void fill(uint8_t proto, uint32_t src, uint32_t dst, uint16_t payload_len) {
        version_ihl    = 0x45;
        dscp_ecn       = 0;
        total_length   = htons((uint16_t)(20 + payload_len));
        id             = 0;
        flags_fragment = htons(0x4000);
        ttl            = 64;
        protocol       = proto;
        checksum       = 0;
        src_ip         = src;
        dst_ip         = dst;
        checksum       = ip_checksum();
    }

    uint16_t ip_checksum() const {
        return fk::algorithms::InternetChecksum::compute(this, 20);
    }
};

static constexpr size_t IPV4_HEADER_SIZE = 20;

} // namespace net
} // namespace fkernel
