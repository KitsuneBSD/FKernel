#pragma once

#include <Kernel/Hardware/Firmware/Acpi/sdt_header.h>
#include <LibFK/Types/types.h>

namespace fkernel::acpi {

struct DMARHeader {
    SDTHeader header;
    uint8_t host_address_width;
    uint8_t flags;
    uint8_t reserved[10];
} __attribute__((packed));

enum class DMARType : uint16_t {
    DRHD = 0, // DMA Remapping Hardware Unit Definition
    RMRR = 1, // Reserved Memory Region Reporting
    ATSR = 2, // Root Port ATS Capability Reporting
    RHSA = 3, // Remapping Hardware Status Affinity
    ANDD = 4, // ACPI Name-space Device Declaration
};

struct DMAREntryHeader {
    DMARType type;
    uint16_t length;
} __attribute__((packed));

struct DMAR_DRHD {
    DMAREntryHeader header;
    uint8_t flags;
    uint8_t reserved;
    uint16_t segment_number;
    uint64_t register_base_address;
} __attribute__((packed));

} // namespace fkernel::acpi
