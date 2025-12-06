#pragma once 

#include <Kernel/Hardware/Acpi/sdt_header.h>
#include <LibFK/Types/types.h>

struct RSDT {
  SDTHeader header;
  uint32_t sdt_pointers[];
} __attribute__((packed));

static_assert(sizeof(RSDT) == 36, "RSDT size must be at least 36 bytes");