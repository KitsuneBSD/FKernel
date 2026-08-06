#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct ExfatFileEntry {
    uint8_t  entry_type;
    uint8_t  secondary_count;
    uint16_t set_checksum;
    uint16_t file_attributes;
    uint8_t  reserved1[2];
    uint32_t create_time;
    uint32_t last_modified_time;
    uint32_t last_accessed_time;
    uint8_t  create_10ms;
    uint8_t  last_modified_10ms;
    uint8_t  create_utc_offset;
    uint8_t  last_modified_utc_offset;
    uint8_t  last_accessed_utc_offset;
    uint8_t  reserved2[7];
} __attribute__((packed));

} // namespace fkernel
