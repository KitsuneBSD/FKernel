#pragma once

#include <Kernel/Driver/Storage/Partitions/Gpt/gpt_header.h>
#include <Kernel/Driver/Storage/Partitions/Gpt/gpt_partition_entry.h>
#include <Kernel/Driver/Storage/Interfaces/storage_device.h>

class GPTParser {
public:
  static fk::core::Result<void, fk::core::Error>
  parse(fk::RefPtr<StorageDevice> device);
};
