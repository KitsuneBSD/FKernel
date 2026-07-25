#pragma once 

#include <Kernel/Hardware/Acpi/sdt_header.h>
#include <LibFK/Types/types.h>

struct Madt {
    SDTHeader header;
    uint32_t lapic_address;
    uint32_t flags;
    uint8_t entries[];
}__attribute__((packed));