#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Net/Core/byte_order.h>

namespace fkernel {
namespace net {

static constexpr uint16_t DHCP_SERVER_PORT = 67;
static constexpr uint16_t DHCP_CLIENT_PORT = 68;
static constexpr size_t   DHCP_MAX_PACKET  = 576;

static constexpr uint8_t DHCP_OP_REQUEST = 1;
static constexpr uint8_t DHCP_OP_REPLY   = 2;
static constexpr uint8_t DHCP_HTYPE_ETH  = 1;
static constexpr uint8_t DHCP_HLEN_ETH   = 6;

// DHCP message types (option 53)
static constexpr uint8_t DHCPDISCOVER = 1;
static constexpr uint8_t DHCPOFFER    = 2;
static constexpr uint8_t DHCPREQUEST  = 3;
static constexpr uint8_t DHCPACK      = 5;
static constexpr uint8_t DHCPNAK      = 6;

// DHCP option codes (RFC 2132)
static constexpr uint8_t DHCP_OPT_SUBNET    = 1;
static constexpr uint8_t DHCP_OPT_ROUTER    = 3;
static constexpr uint8_t DHCP_OPT_DNS       = 6;
static constexpr uint8_t DHCP_OPT_HOSTNAME  = 12;
static constexpr uint8_t DHCP_OPT_REQ_IP    = 50;
static constexpr uint8_t DHCP_OPT_LEASE     = 51;
static constexpr uint8_t DHCP_OPT_MSGTYPE   = 53;
static constexpr uint8_t DHCP_OPT_SERVER_ID = 54;
static constexpr uint8_t DHCP_OPT_END       = 255;

// BOOTP/DHCP fixed header (240 bytes including magic cookie)
struct __attribute__((packed)) DhcpHeader {
    uint8_t  op;
    uint8_t  htype;
    uint8_t  hlen;
    uint8_t  hops;
    uint32_t xid;      // transaction ID (network byte order)
    uint16_t secs;
    uint16_t flags;    // 0x8000 = broadcast flag
    uint32_t ciaddr;   // client IP (network byte order)
    uint32_t yiaddr;   // "your" IP (network byte order)
    uint32_t siaddr;   // server IP (network byte order)
    uint32_t giaddr;   // relay agent IP (network byte order)
    uint8_t  chaddr[16];
    uint8_t  sname[64];
    uint8_t  file[128];
    uint8_t  magic[4]; // must be {99, 130, 83, 99}
    // variable-length options follow
};

static constexpr size_t DHCP_HEADER_SIZE    = sizeof(DhcpHeader); // 240
static constexpr size_t DHCP_MAX_OPTIONS    = DHCP_MAX_PACKET - DHCP_HEADER_SIZE; // 336
static constexpr uint8_t DHCP_MAGIC[4]      = {99, 130, 83, 99};
static constexpr uint16_t DHCP_FLAG_BROADCAST = 0x8000;

} // namespace net
} // namespace fkernel
