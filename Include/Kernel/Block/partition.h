#pragma once

#include <Kernel/Driver/Ata/AtaController.h>
#include <LibFK/Traits/types.h>

struct PartitionEntry {
  uint8_t boot_flag;
  uint8_t chs_first[3];
  uint8_t type;
  uint8_t chs_last[3];
  uint32_t lba_first;
  uint32_t sectors_count;
};