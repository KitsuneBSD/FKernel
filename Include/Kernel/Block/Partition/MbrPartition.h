#pragma once

#include <Kernel/Block/Partition/PartitionParsingStrategy.h>
#include <LibFK/Types/types.h>

struct MbrEntry {
  uint8_t boot_indicator; // 0x80 is bootable
  uint8_t start_chs[3];
  uint8_t partition_type;
  uint8_t end_chs[3];
  uint32_t start_lba;     // little-endian
  uint32_t sectors_count; // little-endian
} __attribute__((packed));

class MbrPartitionStrategy : public PartitionParsingStrategy {
public:
  int parse(const void *sector512, PartitionEntry *out, int max_out) override;
};