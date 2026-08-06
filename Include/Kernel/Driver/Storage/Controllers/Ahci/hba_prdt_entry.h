#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct HBA_PRDT_ENTRY {
    uint32_t dba;
    uint32_t dbau;
    uint32_t reserved0;
    uint32_t dbc:22;
    uint32_t reserved1:9;
    uint32_t i:1;
} __attribute__((packed));

} // namespace fkernel
