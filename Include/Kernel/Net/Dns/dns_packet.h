#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Net/byte_order.h>

namespace fkernel {
namespace net {

static constexpr uint16_t DNS_PORT = 53;
static constexpr size_t DNS_MAX_PACKET = 512;
static constexpr size_t DNS_MAX_NAME = 255;

struct __attribute__((packed)) DnsHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount; // questions
    uint16_t ancount; // answers
    uint16_t nscount; // authority
    uint16_t arcount; // additional

    static constexpr uint16_t FLAG_QR_RESPONSE = 0x8000;
    static constexpr uint16_t FLAG_OPCODE_QUERY = 0x0000;
    static constexpr uint16_t FLAG_RD = 0x0100; // recursion desired
    static constexpr uint16_t FLAG_RA = 0x0080; // recursion available
    static constexpr uint16_t RCODE_MASK = 0x000F;
};

static constexpr uint16_t DNS_TYPE_A = 1;
static constexpr uint16_t DNS_CLASS_IN = 1;

// Encode a hostname into DNS wire format (label-length encoding).
// Returns the number of bytes written, or 0 on error.
// out must be at least DNS_MAX_NAME+2 bytes.
static inline size_t dns_encode_name(const char* host, uint8_t* out, size_t out_max) {
    size_t written = 0;
    const char* p = host;
    while (*p && written + 1 < out_max) {
        const char* dot = p;
        while (*dot && *dot != '.') ++dot;
        size_t label_len = (size_t)(dot - p);
        if (label_len == 0 || label_len > 63) return 0;
        out[written++] = (uint8_t)label_len;
        for (size_t i = 0; i < label_len && written < out_max; ++i)
            out[written++] = (uint8_t)p[i];
        p = (*dot == '.') ? dot + 1 : dot;
    }
    if (written < out_max) out[written++] = 0; // root label
    return written;
}

// Decode the first A-record IPv4 address from a DNS response into `ip_out`.
// Returns true on success.
bool dns_parse_response(const uint8_t* buf, size_t len, uint32_t& ip_out);

} // namespace net
} // namespace fkernel
