#include <Kernel/Memory/VirtualMemory/RegionSplitter/region_splitter.h>

namespace fkernel {

RegionSplitter::RegionSplitter(fk::containers::Vector<MemoryRegion>& regions)
    : m_regions(regions) {}

void RegionSplitter::split(uintptr_t unmap_start, uintptr_t unmap_end) {
    for (size_t i = 0; i < m_regions.size();)
        handle_region(i, unmap_start, unmap_end);
}

void RegionSplitter::handle_region(size_t& i, uintptr_t unmap_start, uintptr_t unmap_end) {
    auto& region = m_regions[i];
    if (is_fully_outside(region, unmap_start, unmap_end)) { ++i; return; }
    if (is_fully_inside(region, unmap_start, unmap_end)) { erase_at(i); return; }
    if (cuts_head(region, unmap_start, unmap_end)) { region.end = unmap_start; ++i; return; }
    if (cuts_tail(region, unmap_start, unmap_end)) { region.start = unmap_end; ++i; return; }
    MemoryRegion right = region;
    right.start = unmap_end;
    region.end = unmap_start;
    m_regions.push_back(fk::types::move(right));
    ++i;
}

bool RegionSplitter::is_fully_outside(const MemoryRegion& r, uintptr_t start, uintptr_t end) const {
    return r.end <= start || r.start >= end;
}

bool RegionSplitter::is_fully_inside(const MemoryRegion& r, uintptr_t start, uintptr_t end) const {
    return r.start >= start && r.end <= end;
}

bool RegionSplitter::cuts_head(const MemoryRegion& r, uintptr_t start, uintptr_t end) const {
    return r.start < start && r.end <= end && r.end > start;
}

bool RegionSplitter::cuts_tail(const MemoryRegion& r, uintptr_t start, uintptr_t end) const {
    return r.start >= start && r.start < end && r.end > end;
}

void RegionSplitter::erase_at(size_t i) {
    for (size_t j = i; j < m_regions.size() - 1; ++j)
        m_regions[j] = fk::types::move(m_regions[j + 1]);
    m_regions.pop_back();
}

} // namespace fkernel
