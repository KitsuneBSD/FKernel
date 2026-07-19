#pragma once

#include <LibC/stdint.h>

namespace fkernel {
namespace net {

static inline uint16_t htons(uint16_t v) {
    return (uint16_t)((v >> 8) | (v << 8));
}

static inline uint16_t ntohs(uint16_t v) { return htons(v); }

static inline uint32_t htonl(uint32_t v) {
    return ((v & 0xFF000000u) >> 24) | ((v & 0x00FF0000u) >> 8)
         | ((v & 0x0000FF00u) << 8)  | ((v & 0x000000FFu) << 24);
}

static inline uint32_t ntohl(uint32_t v) { return htonl(v); }

} // namespace net
} // namespace fkernel
