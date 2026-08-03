#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace algorithms {

// Raw byte-array bitmap helpers for on-disk bitmaps (ext2, minix, exfat, ...).
// Bit `i` lives in byte `i / 8`, at bit position `i % 8` (LSB-first).

inline bool test_bit(const uint8_t* bitmap, size_t bit) {
    return (bitmap[bit / 8] >> (bit % 8)) & 1;
}

inline void set_bit(uint8_t* bitmap, size_t bit) {
    bitmap[bit / 8] |= static_cast<uint8_t>(1u << (bit % 8));
}

inline void clear_bit(uint8_t* bitmap, size_t bit) {
    bitmap[bit / 8] &= static_cast<uint8_t>(~(1u << (bit % 8)));
}

// First free (clear) bit at or after `start`, scanning byte-wise with the
// full-byte fast path. Returns nbits when the range has no free bit.
inline size_t find_first_free_bit(const uint8_t* bitmap, size_t nbits, size_t start = 0) {
    if (start >= nbits) return nbits;
    for (size_t byte = start / 8; byte < (nbits + 7) / 8; ++byte) {
        uint8_t w = bitmap[byte];
        if (w == 0xFF) continue;
        size_t first = (byte == start / 8) ? (start % 8) : 0;
        for (size_t bit = first; bit < 8; ++bit) {
            size_t index = byte * 8 + bit;
            if (index >= nbits) return nbits;
            if (!(w & (1u << bit))) return index;
        }
    }
    return nbits;
}

} // namespace algorithms
} // namespace fk
