#pragma once

#include <Kernel/Memory/VirtualMemory/memory_region.h>
#include <LibFK/Container/Sequence/vector.h>

namespace fkernel {

class RegionSplitter {
  fk::containers::Vector<MemoryRegion>& m_regions;

public:
  explicit RegionSplitter(fk::containers::Vector<MemoryRegion>& regions);
  void split(uintptr_t unmap_start, uintptr_t unmap_end);

private:
  void handle_region(size_t& i, uintptr_t unmap_start, uintptr_t unmap_end);
  bool is_fully_outside(const MemoryRegion& r, uintptr_t start, uintptr_t end) const;
  bool is_fully_inside(const MemoryRegion& r, uintptr_t start, uintptr_t end) const;
  bool cuts_head(const MemoryRegion& r, uintptr_t start, uintptr_t end) const;
  bool cuts_tail(const MemoryRegion& r, uintptr_t start, uintptr_t end) const;
  void erase_at(size_t i);
};

} // namespace fkernel
