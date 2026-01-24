#pragma once 

#include <Kernel/Hardware/Madt/madt_entries.h>

struct MADT_IOAPIC {
    MadtEntry header;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t address;
    uint32_t gsi_base;
};