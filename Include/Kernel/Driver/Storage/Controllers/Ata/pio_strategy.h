#pragma once

#include <Kernel/Driver/Storage/Controllers/Ata/ata_transfer_strategy.h>
#include <LibFK/Types/types.h>

class PIOStrategy : public ATATransferStrategy {
  uint16_t m_io_base;
  [[maybe_unused]] uint16_t m_control_base;
  bool m_is_master;

public:
  PIOStrategy(uint16_t io_base, uint16_t control_base, bool is_master)
      : m_io_base(io_base), m_control_base(control_base),
        m_is_master(is_master) {}

  virtual fk::core::Result<size_t, fk::core::Error>
  read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) override;

  virtual fk::core::Result<size_t, fk::core::Error>
  write_sectors(uint64_t start_sector, size_t count,
                const uint8_t *buffer) override;

  virtual const char *name() const override { return "PIO"; }

private:
  void select_drive(uint64_t lba, uint8_t count);
  void wait_busy();
  void wait_ready();
  bool wait_status(uint8_t mask, uint8_t value,
                   uint32_t timeout_loops = 1000000);
};
