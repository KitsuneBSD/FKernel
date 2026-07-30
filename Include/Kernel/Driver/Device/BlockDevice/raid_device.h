#pragma once

#include <Kernel/Driver/Device/BlockDevice/stackable_block_device.h>

namespace fkernel {

enum class RaidMode : uint8_t {
  Stripe = 0, // RAID 0: stripe across all children
  Mirror = 1, // RAID 1: mirror to all, read round-robin
};

class RaidDevice : public StackableBlockDevice {
  RaidMode m_mode;
  size_t   m_stripe_sectors; // RAID 0: sectors per stripe chunk
  size_t   m_read_idx{0};    // RAID 1: round-robin read index

public:
  RaidDevice(RaidMode mode, size_t stripe_sectors)
      : m_mode(mode), m_stripe_sectors(stripe_sectors) {}
  static fk::RefPtr<RaidDevice> create_raid0(size_t stripe_sectors = 128);
  static fk::RefPtr<RaidDevice> create_raid1();

  fk::core::Result<size_t, fk::core::Error>
  read_sectors(uint64_t start, size_t count, uint8_t* buf) override;

  fk::core::Result<size_t, fk::core::Error>
  write_sectors(uint64_t start, size_t count, const uint8_t* buf) override;

  SectorSize  sector_size()  const override;
  SectorCount sector_count() const override;
};

} // namespace fkernel
