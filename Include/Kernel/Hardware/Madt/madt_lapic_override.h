#pragma once 

#include <Kernel/Hardware/Madt/madt_entries.h>
#include <LibFK/Types/types.h>

struct MADT_LAPIC_OVERRIDE {
    MadtEntry header;
    uint16_t reserved;
    uint64_t lapic_address;
};