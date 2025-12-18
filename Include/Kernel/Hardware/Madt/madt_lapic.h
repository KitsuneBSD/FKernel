#pragma once 

#include <Kernel/Hardware/Madt/madt_entries.h>
#include <LibFK/Types/types.h>

struct MADT_LAPIC {
    MadtEntry header;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags; // TODO: Change to enum class with available flags 
};