#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct ExfatStreamExtEntry {
    uint8_t  entry_type;
    uint8_t  general_secondary_flags;
    uint8_t  reserved1;
    uint8_t  name_length;
    uint16_t name_hash;
    uint8_t  reserved2[2];
    uint64_t valid_data_length;
    uint8_t  reserved3[4];
    uint32_t first_cluster;
    uint64_t data_length;
} __attribute__((packed));

} // namespace fkernel
