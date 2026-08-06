#pragma once
#include <Kernel/Hardware/Firmware/Acpi/srat_entry_header.h>
#include <LibFK/Types/types.h>
namespace fkernel::acpi {
struct SRATProcessorLocalApic {
    SRATEntryHeader header;
    uint8_t proximity_domain_low;
    uint8_t apic_id;
    uint32_t flags;
    uint8_t local_sapic_eid;
    uint8_t proximity_domain_high[3];
    uint32_t clock_domain;
    uint32_t proximity_domain() const {
        return proximity_domain_low |
               (static_cast<uint32_t>(proximity_domain_high[0]) << 8) |
               (static_cast<uint32_t>(proximity_domain_high[1]) << 16) |
               (static_cast<uint32_t>(proximity_domain_high[2]) << 24);
    }
} __attribute__((packed));
} // namespace fkernel::acpi
