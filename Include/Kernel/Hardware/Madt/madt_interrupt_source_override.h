#pragma once 

#include <Kernel/Hardware/Madt/madt_entries.h>
#include <LibFK/Types/types.h>

struct MADT_InterruptSourceOverride {
    MadtEntry header;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t gsi;
    uint16_t flags;
};