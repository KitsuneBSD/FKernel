#pragma once

#include <LibFK/Types/types.h>

struct GPTPartitionEntry {
  uint8_t partition_type_guid[16];
  uint8_t unique_partition_guid[16];
  uint64_t starting_lba;
  uint64_t ending_lba;
  uint64_t attributes;
  char16_t partition_name[36];
} __attribute__((packed));
