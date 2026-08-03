#pragma once

#include <Kernel/Hardware/Buses/Pci/pci_event_type.h>
#include <LibFK/Types/types.h>

namespace fkernel {

struct PCIEvent {
    PCIEventType type;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor;
    uint16_t device_id;
};

} // namespace fkernel
