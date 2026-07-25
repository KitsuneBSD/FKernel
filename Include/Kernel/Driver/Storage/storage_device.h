#pragma once

#include <Kernel/Driver/Device/BlockDevice/block_device.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

using fkernel::SectorSize;
using fkernel::SectorCount;

class StorageDevice : public fkernel::BlockDevice {
public:
  virtual ~StorageDevice() override = default;

  virtual fk::core::Result<size_t, fk::core::Error>
  read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) override = 0;
  virtual fk::core::Result<size_t, fk::core::Error>
  write_sectors(uint64_t start_sector, size_t count, const uint8_t *buffer) override = 0;

  virtual SectorSize sector_size() const override = 0;
  virtual SectorCount sector_count() const override = 0;

  // name() is already in Node

protected:
  StorageDevice() = default;
};
