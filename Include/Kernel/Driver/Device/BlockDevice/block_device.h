#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Types/types.h>

#include <Kernel/Driver/Device/BlockDevice/sector_count.h>
#include <Kernel/Driver/Device/BlockDevice/sector_size.h>

namespace fkernel {

class BlockDevice : public Node {
public:
  virtual ~BlockDevice() override = default;

  virtual fk::core::Result<size_t, fk::core::Error>
  read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) = 0;
  virtual fk::core::Result<size_t, fk::core::Error>
  write_sectors(uint64_t start_sector, size_t count, const uint8_t *buffer) = 0;

  virtual SectorSize sector_size() const = 0;
  virtual SectorCount sector_count() const = 0;

  virtual bool is_block_device() const override { return true; }

  // From Node
  virtual fk::core::Result<size_t, fk::core::Error>
  read(uint64_t offset, size_t size, uint8_t *buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error>
  write(uint64_t offset, size_t size, const uint8_t *buffer) override;
  virtual size_t size() const override;

protected:
  BlockDevice() = default;
};

} // namespace fkernel
