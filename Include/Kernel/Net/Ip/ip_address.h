#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Text/string_builder.h>
#include <Kernel/Net/byte_order.h>

namespace fkernel {
namespace net {

struct IPv4Address {
    uint32_t value; // host byte order

    IPv4Address() : value(0) {}
    explicit IPv4Address(uint32_t v) : value(v) {}
    IPv4Address(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
        : value((uint32_t)a << 24 | (uint32_t)b << 16 | (uint32_t)c << 8 | d) {}

    uint32_t to_network() const { return htonl(value); }
    static IPv4Address from_network(uint32_t n) { return IPv4Address(ntohl(n)); }

    bool operator==(const IPv4Address& o) const { return value == o.value; }
    bool operator!=(const IPv4Address& o) const { return value != o.value; }

    fk::text::String to_string() const {
        fk::text::StringBuilder sb;
        sb.append_decimal((uint64_t)((value >> 24) & 0xFF)); sb.append('.');
        sb.append_decimal((uint64_t)((value >> 16) & 0xFF)); sb.append('.');
        sb.append_decimal((uint64_t)((value >>  8) & 0xFF)); sb.append('.');
        sb.append_decimal((uint64_t)(value & 0xFF));
        return sb.to_string();
    }

    static IPv4Address broadcast() { return IPv4Address(0xFFFFFFFFu); }
    static IPv4Address any()       { return IPv4Address(0u); }
};

} // namespace net
} // namespace fkernel
