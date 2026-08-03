#pragma once

#include <Kernel/Hardware/Firmware/Acpi/sdt_header.h>

struct MCFGEntry {
    uint64_t base_address;
    uint16_t pci_segment_group_number;
    uint8_t start_bus_number;
    uint8_t end_bus_number;
    uint32_t reserved;
} __attribute__((packed));

struct MCFGTable {
    SDTHeader header;
    uint64_t reserved;
    MCFGEntry entries[];
} __attribute__((packed));
