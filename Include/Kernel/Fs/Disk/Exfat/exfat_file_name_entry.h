#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct ExfatFileNameEntry {
    uint8_t  entry_type;
    uint8_t  general_secondary_flags;
    uint16_t file_name[15];
} __attribute__((packed));

} // namespace fkernel
