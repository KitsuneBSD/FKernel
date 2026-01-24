#pragma once 

#include <LibFK/Types/types.h>

struct RSDP {
  char signature[8];
  uint8_t checksum;
  char oem_id[6];
  uint8_t revision;
  uint32_t rsdt_address;

  // ACPI 2.0 fields
  uint32_t length;
  uint64_t xsdt_address;
  uint8_t extended_checksum;
  uint8_t reserved[3];
} __attribute__((packed));

static_assert(sizeof(RSDP) == 36, "RSDP structure size mismatch");