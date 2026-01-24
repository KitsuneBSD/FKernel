#pragma once

#include <Kernel/Driver/Storage/storage_device.h>
#include <LibFK/Memory/retain_ptr.h>

class Partition : public StorageDevice {
  fk::text::String m_name;
  fk::RefPtr<StorageDevice> m_parent_device;
  uint64_t m_start_sector;
  SectorCount m_partition_sector_count;

public:
  static fk::core::Result<fk::RefPtr<Partition>, fk::core::Error>
  create(fk::RefPtr<StorageDevice> parent, uint64_t start,
         uint64_t count, fk::text::String name);

  Partition(fk::RefPtr<StorageDevice> parent, uint64_t start,
            uint64_t count, fk::text::String name);

  virtual fk::core::Result<size_t, fk::core::Error>
  read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error>
  write_sectors(uint64_t start_sector, size_t count,
                const uint8_t *buffer) override;

  virtual SectorSize sector_size() const override {
    return m_parent_device->sector_size();
  }
  virtual SectorCount sector_count() const override {
    return m_partition_sector_count;
  }
  virtual fk::text::String name() const override { return m_name; }

  uint64_t start_sector() const { return m_start_sector; }

private:
  bool is_within_bounds(uint64_t start, size_t count) const;
};
