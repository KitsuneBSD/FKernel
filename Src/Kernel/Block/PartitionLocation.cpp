#include <Kernel/Block/PartitionLocation.h>

PartitionLocation::PartitionLocation(uint32_t lba_first, uint32_t sectors_count)
    : m_lba_first(lba_first)
    , m_sectors_count(sectors_count) {}

uint32_t PartitionLocation::first_lba() const {
    return m_lba_first;
}

uint32_t PartitionLocation::sector_count() const {
    return m_sectors_count;
}

size_t PartitionLocation::size_in_bytes() const {
    return (size_t)m_sectors_count * 512;
}

uint64_t PartitionLocation::absolute_offset(size_t relative_offset) const {
    return (uint64_t)m_lba_first * 512 + relative_offset;
}
