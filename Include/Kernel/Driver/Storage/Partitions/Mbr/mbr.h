#pragma once

#include <Kernel/Driver/Storage/Partitions/Mbr/mbr_header.h>
#include <Kernel/Driver/Storage/Partitions/Mbr/mbr_partition_entry.h>
#include <Kernel/Driver/Storage/storage_device.h>
#include <LibFK/Container/vector.h>

class MBRParser {
public:
  static fk::core::Result<void, fk::core::Error>
  parse(fk::RefPtr<StorageDevice> device);

private:
  static fk::core::Result<void, fk::core::Error>
  parse_extended(fk::RefPtr<StorageDevice> device, uint32_t ebr_lba,
                 uint32_t base_ebr_lba, int& partition_count, int depth = 0);
};
