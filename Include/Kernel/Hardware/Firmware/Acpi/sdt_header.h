#pragma once 

#include <LibFK/Types/types.h>

struct SDTHeader {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oem_id[6];
  char oem_table_id[8];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
} __attribute__((packed));

static_assert(sizeof(SDTHeader) == 36, "SDTHeader size must be 36 bytes");