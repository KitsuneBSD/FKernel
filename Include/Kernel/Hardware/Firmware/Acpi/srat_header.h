#pragma once
#include <Kernel/Hardware/Firmware/Acpi/sdt_header.h>
#include <LibFK/Types/types.h>
namespace fkernel::acpi {
struct SRATHeader {
    SDTHeader header;
    uint32_t reserved1;
    uint64_t reserved2;
} __attribute__((packed));
} // namespace fkernel::acpi
