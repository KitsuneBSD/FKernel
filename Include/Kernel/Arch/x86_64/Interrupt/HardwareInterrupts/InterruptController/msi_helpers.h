#pragma once

#include <LibFK/Core/result.h>
#include <LibFK/Types/types.h>

class PciDevice;

namespace msi {

// Returns the physical base of the local APIC from IA32_APIC_BASE MSR (0x1B).
// Valid for both xAPIC and x2APIC modes.
uint32_t lapic_phys_address();

fk::core::Result<uint8_t, fk::core::Error>
write_msi_config(const PciDevice& device, uint32_t msi_addr);

fk::core::Result<uint8_t, fk::core::Error>
allocate_vector();

// Convenience: resolve LAPIC address via MSR then write MSI config.
fk::core::Result<uint8_t, fk::core::Error>
allocate_msi_vector(const PciDevice& device);

} // namespace msi
