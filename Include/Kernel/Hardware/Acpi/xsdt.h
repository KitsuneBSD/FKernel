#pragma once 

#include <LibFK/Types/types.h>
#include <Kernel/Hardware/Acpi/sdt_header.h>

struct XSDT {
  SDTHeader header;
  uint64_t sdt_pointers[];
} __attribute__((packed));

static_assert(sizeof(XSDT) == 36, "XSDT size must be at least 36 bytes");