#include <Kernel/Net/Icmp/icmp_packet.h>
#include <LibC/stddef.h>

namespace fkernel {
namespace net {

uint16_t IcmpHeader::compute_checksum(const uint8_t* payload, size_t payload_len) const {
    uint32_t sum = 0;
    const uint16_t* hdr_words = reinterpret_cast<const uint16_t*>(this);
    for (size_t i = 0; i < ICMP_HEADER_SIZE / 2; ++i) sum += hdr_words[i];
    const uint16_t* p = reinterpret_cast<const uint16_t*>(payload);
    size_t word_count = payload_len / 2;
    for (size_t i = 0; i < word_count; ++i) sum += p[i];
    if (payload_len % 2) sum += (uint16_t)payload[payload_len - 1];
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)(~sum);
}

} // namespace net
} // namespace fkernel
