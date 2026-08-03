#pragma once

#include <LibFK/Types/types.h>

struct e1000_rx_desc {
  uint64_t addr;
  uint16_t len;
  uint16_t checksum;
  uint8_t status;
  uint8_t errors;
  uint16_t special;
} __attribute__((packed));
