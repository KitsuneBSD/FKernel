#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Driver/Network/mac_address.h>

namespace fkernel {
namespace net {

static constexpr uint16_t ETHERTYPE_IPV4 = 0x0800;
static constexpr uint16_t ETHERTYPE_ARP  = 0x0806;

struct __attribute__((packed)) EthernetHeader {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype; // big-endian

    void set_dst(const MACAddress& mac) {
        for (int i = 0; i < 6; ++i) dst_mac[i] = mac.address[i];
    }
    void set_src(const MACAddress& mac) {
        for (int i = 0; i < 6; ++i) src_mac[i] = mac.address[i];
    }
    MACAddress get_src() const {
        return MACAddress(src_mac[0], src_mac[1], src_mac[2],
                          src_mac[3], src_mac[4], src_mac[5]);
    }
    MACAddress get_dst() const {
        return MACAddress(dst_mac[0], dst_mac[1], dst_mac[2],
                          dst_mac[3], dst_mac[4], dst_mac[5]);
    }
};

static constexpr size_t ETHERNET_HEADER_SIZE = 14;

} // namespace net
} // namespace fkernel
