#pragma once

#include <Kernel/Driver/Storage/Controllers/Ahci/hba_port.h>
#include <Kernel/Driver/Async/dma_buffer.h>
#include <LibFK/Types/types.h>

namespace fkernel {

struct AhciPort {
    uint32_t index;
    bool is_implemented;
    bool has_device;
    uint32_t sig;
    uint64_t sectors;
    volatile HBA_PORT* regs;
    DmaBuffer cmd_list;
    DmaBuffer fis_buffer;
    DmaBuffer cmd_tables;
};

} // namespace fkernel
