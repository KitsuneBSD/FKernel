#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct ExfatAllocBitmapEntry {
    uint8_t  entry_type;
    uint8_t  bitmap_flags;
    uint8_t  reserved[18];
    uint32_t first_cluster;
    uint64_t data_length;
} __attribute__((packed));

} // namespace fkernel
