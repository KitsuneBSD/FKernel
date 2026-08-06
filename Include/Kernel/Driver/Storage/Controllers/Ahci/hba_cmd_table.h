#pragma once

#include <Kernel/Driver/Storage/Controllers/Ahci/hba_prdt_entry.h>
#include <LibFK/Types/types.h>

namespace fkernel {

struct HBA_CMD_TABLE {
    uint8_t         cfis[64];
    uint8_t         acmd[16];
    uint8_t         reserved[48];
    HBA_PRDT_ENTRY  prdt_entry[1];
} __attribute__((packed));

} // namespace fkernel
