#pragma once
#include <Kernel/Hardware/Firmware/Acpi/srat_entry_header.h>
#include <LibFK/Types/types.h>
namespace fkernel::acpi {
struct SRATProcessorLocalX2Apic {
    SRATEntryHeader header;
    uint16_t reserved1;
    uint32_t proximity_domain;
    uint32_t x2apic_id;
    uint32_t flags;
    uint32_t clock_domain;
    uint32_t reserved2;
} __attribute__((packed));
} // namespace fkernel::acpi
