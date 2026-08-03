#pragma once

#include <Kernel/Driver/Storage/Partitions/Mbr/mbr_partition_entry.h>
#include <Kernel/Driver/Storage/Interfaces/storage_device.h>
#include <LibFK/Container/Sequence/vector.h>

struct MBRHeader {
  uint8_t boot_code[446];
  MBRPartitionEntry entries[4];
  uint16_t signature;
} __attribute__((packed));
