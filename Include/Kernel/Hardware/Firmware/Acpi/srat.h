#pragma once

#include <Kernel/Hardware/Firmware/Acpi/sdt_header.h>
#include <LibFK/Types/types.h>

namespace fkernel::acpi {

struct SRATHeader {
    SDTHeader header;
    uint32_t reserved1;
    uint64_t reserved2;
} __attribute__((packed));

enum class SRATEntryType : uint8_t {
    ProcessorLocalApic = 0,
    MemoryAffinity = 1,
    ProcessorLocalX2Apic = 2,
    GicItAffinity = 3,
};

struct SRATEntryHeader {
    SRATEntryType type;
    uint8_t length;
} __attribute__((packed));

struct SRATProcessorLocalApic {
    SRATEntryHeader header;
    uint8_t proximity_domain_low;
    uint8_t apic_id;
    uint32_t flags;
    uint8_t local_sapic_eid;
    uint8_t proximity_domain_high[3];
    uint32_t clock_domain;

    uint32_t proximity_domain() const {
        return proximity_domain_low | (static_cast<uint32_t>(proximity_domain_high[0]) << 8) |
               (static_cast<uint32_t>(proximity_domain_high[1]) << 16) | (static_cast<uint32_t>(proximity_domain_high[2]) << 24);
    }
} __attribute__((packed));

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
    uint64_t length() const { return (static_cast<uint64_t>(length_high) << 32) | length_low; }
    bool is_enabled() const { return flags & 1; }
    bool is_hot_pluggable() const { return flags & 2; }
    bool is_non_volatile() const { return flags & 4; }
} __attribute__((packed));

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
