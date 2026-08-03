#include <Kernel/Net/Icmp/icmp_packet.h>
#include <LibFK/Types/types.h>
#include <LibFK/Algorithms/Crypto/internet_checksum.h>

namespace fkernel {
namespace net {

uint16_t IcmpHeader::compute_checksum(const uint8_t* payload, size_t payload_len) const {
    fk::algorithms::InternetChecksum c;
    c.accumulate(this, ICMP_HEADER_SIZE);
    c.accumulate(payload, payload_len);
    return c.finalize();
}

} // namespace net
} // namespace fkernel
