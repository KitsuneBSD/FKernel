#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Represents a single partition table entry in the MBR (Master Boot
 * Record)
 */
struct PartitionEntry {
  uint8_t type;
  uint32_t lba_start;
  uint32_t lba_count;
  bool is_bootable;
  bool has_chs;
} __attribute__((packed));
