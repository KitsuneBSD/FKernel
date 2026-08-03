#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace algorithms {

static inline uint16_t swap16(uint16_t v) {
    return (uint16_t)((v >> 8) | (v << 8));
}

static inline uint32_t swap32(uint32_t v) {
    return ((v & 0xFF000000u) >> 24) | ((v & 0x00FF0000u) >> 8)
         | ((v & 0x0000FF00u) << 8)  | ((v & 0x000000FFu) << 24);
}

static inline uint64_t swap64(uint64_t v) {
    return ((uint64_t)swap32((uint32_t)v) << 32) | swap32((uint32_t)(v >> 32));
}

static inline uint16_t htons(uint16_t v) { return swap16(v); }
static inline uint16_t ntohs(uint16_t v) { return swap16(v); }

static inline uint32_t htonl(uint32_t v) { return swap32(v); }
static inline uint32_t ntohl(uint32_t v) { return swap32(v); }

static inline uint64_t htonll(uint64_t v) { return swap64(v); }
static inline uint64_t ntohll(uint64_t v) { return swap64(v); }

// Byte-buffer loads (used for on-disk structures). The buffer holds raw bytes
// in the given endianness; values are returned in host order. Unaligned-safe.
static inline uint16_t load_le16(const uint8_t* buf, size_t off) {
    return (uint16_t)((uint16_t)buf[off] | ((uint16_t)buf[off + 1] << 8));
}
static inline uint32_t load_le32(const uint8_t* buf, size_t off) {
    return (uint32_t)buf[off]
         | ((uint32_t)buf[off + 1] << 8)
         | ((uint32_t)buf[off + 2] << 16)
         | ((uint32_t)buf[off + 3] << 24);
}
static inline uint64_t load_le64(const uint8_t* buf, size_t off) {
    return (uint64_t)load_le32(buf, off)
         | ((uint64_t)load_le32(buf, off + 4) << 32);
}
static inline uint16_t load_be16(const uint8_t* buf, size_t off) {
    return (uint16_t)(((uint16_t)buf[off] << 8) | (uint16_t)buf[off + 1]);
}
static inline uint32_t load_be32(const uint8_t* buf, size_t off) {
    return ((uint32_t)buf[off] << 24)
         | ((uint32_t)buf[off + 1] << 16)
         | ((uint32_t)buf[off + 2] << 8)
         | (uint32_t)buf[off + 3];
}
static inline uint64_t load_be64(const uint8_t* buf, size_t off) {
    return ((uint64_t)load_be32(buf, off) << 32)
         | load_be32(buf, off + 4);
}

} // namespace algorithms
} // namespace fk
