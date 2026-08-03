#pragma once

#include <LibFK/Types/types.h>

struct e1000_tx_desc {
  uint64_t addr;
  uint16_t len;
  uint8_t lower_setup;
  uint8_t upper_setup;
  uint8_t status;
  uint8_t css;
  uint16_t special;
} __attribute__((packed));
