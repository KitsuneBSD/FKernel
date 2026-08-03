#pragma once

#include <Kernel/Driver/Storage/Interfaces/storage_device.h>
#include <LibFK/Container/Sequence/vector.h>

struct MBRPartitionEntry {
  uint8_t bootable;
  uint8_t start_head;
  uint8_t start_sector : 6;
  uint16_t start_cylinder : 10;
  uint8_t system_id;
  uint8_t end_head;
  uint8_t end_sector : 6;
  uint16_t end_cylinder : 10;
  uint32_t lba_start;
  uint32_t lba_length;
} __attribute__((packed));
