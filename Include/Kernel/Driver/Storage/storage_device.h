#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

class SectorSize {
  size_t m_value;

public:
  explicit SectorSize(size_t size) : m_value(size) {}
  size_t value() const { return m_value; }
};

class SectorCount {
  uint64_t m_value;

public:
  explicit SectorCount(uint64_t count) : m_value(count) {}
  uint64_t value() const { return m_value; }
};

class StorageDevice : public Node {
public:
  virtual ~StorageDevice() override = default;

  virtual fk::core::Result<size_t, fk::core::Error>
  read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) = 0;
  virtual fk::core::Result<size_t, fk::core::Error>
  write_sectors(uint64_t start_sector, size_t count, const uint8_t *buffer) = 0;

  virtual SectorSize sector_size() const = 0;
  virtual SectorCount sector_count() const = 0;
  virtual fk::text::String name() const = 0;

  // From Node
  virtual fk::core::Result<size_t, fk::core::Error>
  read(uint64_t offset, size_t size, uint8_t *buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error>
  write(uint64_t offset, size_t size, const uint8_t *buffer) override;
  virtual size_t size() const override;

protected:
  StorageDevice() = default;
};
