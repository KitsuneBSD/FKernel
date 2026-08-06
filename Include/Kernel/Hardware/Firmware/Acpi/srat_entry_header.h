#pragma once
#include <Kernel/Hardware/Firmware/Acpi/srat_entry_type.h>
#include <LibFK/Types/types.h>
namespace fkernel::acpi {
struct SRATEntryHeader {
    SRATEntryType type;
    uint8_t length;
} __attribute__((packed));
} // namespace fkernel::acpi
