#pragma once

#include <LibFK/Types/types.h>

class PartitionLocation {
public:
    PartitionLocation(uint32_t lba_first, uint32_t sectors_count);

    uint32_t first_lba() const;
    uint32_t sector_count() const;
    size_t size_in_bytes() const;
    uint64_t absolute_offset(size_t relative_offset) const;

private:
    uint32_t m_lba_first;
    uint32_t m_sectors_count;
};
