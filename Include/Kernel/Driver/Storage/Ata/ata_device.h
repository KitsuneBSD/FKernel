#pragma once

#include <Kernel/Driver/Storage/Ata/ata_transfer_strategy.h>
#include <Kernel/Driver/Storage/storage_device.h>
#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Text/string.h>

class ATADevice : public StorageDevice {
  fk::memory::OwnPtr<ATATransferStrategy> m_strategy;
  SectorCount m_sectors;
  SectorSize m_sector_size;

public:
  ATADevice(fk::text::String name,
            fk::memory::OwnPtr<ATATransferStrategy> strategy,
            SectorCount sectors)
      : m_strategy(fk::types::move(strategy)),
        m_sectors(sectors), m_sector_size(512) {
    set_name(fk::types::move(name));
  }

  virtual fk::core::Result<size_t, fk::core::Error>
  read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) override;

  virtual fk::core::Result<size_t, fk::core::Error>
  write_sectors(uint64_t start_sector, size_t count,
                const uint8_t *buffer) override;

  virtual SectorSize sector_size() const override { return m_sector_size; }
  virtual SectorCount sector_count() const override { return m_sectors; }

  const char *strategy_name() const { return m_strategy->name(); }
};
