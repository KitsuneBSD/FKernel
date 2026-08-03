#pragma once

#include <Kernel/Driver/Device/BlockDevice/lvm_segment.h>
#include <Kernel/Driver/Device/BlockDevice/stackable_block_device.h>
#include <LibFK/Container/Sequence/vector.h>

namespace fkernel {

// Linear LVM logical volume: segments map LV extents → (PV, PV offset).
// Striped LVs are modelled by adding multiple single-sector-wide segments
// via add_stripe_segments().
class LvmDevice : public StackableBlockDevice {
  fk::containers::Vector<LvmSegment> m_segments;
  uint64_t m_total_sectors{0};

  const LvmSegment* find_segment(uint64_t lv_sector) const;

public:
  LvmDevice() = default;
  static fk::RefPtr<LvmDevice> create();

  // Append a linear segment mapping lv_start..lv_start+length → pv_index:pv_start.
  void add_segment(uint64_t lv_start, uint64_t length, size_t pv_index, uint64_t pv_start);

  // Convenience: build a striped LV by distributing extents round-robin
  // across pv_count PVs starting from pv_base_start[] on each PV.
  // extent_size_sectors: LVM extent size (default 8192 = 4MB with 512B sectors).
  void add_stripe_segments(uint64_t lv_start_sectors, uint64_t total_sectors,
                           size_t pv_count, const uint64_t* pv_base_starts,
                           uint64_t extent_size_sectors = 8192);

  fk::core::Result<size_t, fk::core::Error>
  read_sectors(uint64_t start, size_t count, uint8_t* buf) override;

  fk::core::Result<size_t, fk::core::Error>
  write_sectors(uint64_t start, size_t count, const uint8_t* buf) override;

  SectorSize  sector_size()  const override;
  SectorCount sector_count() const override;
};

} // namespace fkernel
