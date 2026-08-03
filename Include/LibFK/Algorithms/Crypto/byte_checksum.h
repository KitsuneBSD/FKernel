#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace algorithms {

// ACPI-style table validation: sum of all bytes must be zero (mod 256).
static inline bool byte_checksum_valid(const void* data, size_t len) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    uint8_t sum = 0;
    for (size_t i = 0; i < len; ++i) sum += p[i];
    return sum == 0;
}

// Compute the checksum byte that makes the sum of all bytes in `data` (length
// `len`) equal zero mod 256. Used when writing ACPI tables.
static inline uint8_t byte_checksum_for(const void* data, size_t len) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    uint8_t sum = 0;
    for (size_t i = 0; i < len; ++i) sum += p[i];
    return (uint8_t)(0x100u - sum);
}

} // namespace algorithms
} // namespace fk
