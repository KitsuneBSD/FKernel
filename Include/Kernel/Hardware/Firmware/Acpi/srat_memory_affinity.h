#pragma once
#include <Kernel/Hardware/Firmware/Acpi/srat_entry_header.h>
#include <LibFK/Types/types.h>
namespace fkernel::acpi {
struct SRATMemoryAffinity {
    SRATEntryHeader header;
    uint32_t proximity_domain;
    uint16_t reserved1;
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t reserved2;
    uint32_t flags;
    uint64_t reserved3;
    uint64_t base_addr() const { return (static_cast<uint64_t>(base_addr_high) << 32) | base_addr_low; }
    uint64_t length()    const { return (static_cast<uint64_t>(length_high) << 32) | length_low; }
    bool is_enabled()       const { return flags & 1; }
    bool is_hot_pluggable() const { return flags & 2; }
    bool is_non_volatile()  const { return flags & 4; }
} __attribute__((packed));
} // namespace fkernel::acpi
