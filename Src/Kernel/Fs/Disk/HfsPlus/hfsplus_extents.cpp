#include <Kernel/Fs/Disk/HfsPlus/hfsplus_extents.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/Generic/byte_order.h>
#include <LibFK/Algorithms/Logging/log.h>

namespace fkernel {

void HFSPlusForkReader::set(
    fk::RefPtr<StorageDevice> device,
    const BTreeFile*           extents_tree,
    const HFSPlusForkData&     fork,
    uint32_t                   block_size,
    uint32_t                   first_sector,
    uint32_t                   file_id,
    uint8_t                    fork_type)
{
    m_device        = device;
    m_extents_tree  = extents_tree;
    m_fork          = fork;
    m_block_size    = block_size;
    m_first_sector  = first_sector;
    m_file_id       = file_id;
    m_fork_type     = fork_type;
    m_logical_size  = fk::algorithms::swap64(fork.logicalSize);
    m_total_blocks  = fk::algorithms::swap32(fork.totalBlocks);
}

fk::core::Result<uint32_t, fk::core::Error>
HFSPlusForkReader::resolve_block(uint32_t logical_block) const
{
    // First, try the 8 inline extents
    uint32_t remaining = logical_block;
    for (int i = 0; i < kHFSPlusExtentDensity; ++i) {
        uint32_t start = fk::algorithms::swap32(m_fork.extents[i].startBlock);
        uint32_t count = fk::algorithms::swap32(m_fork.extents[i].blockCount);
        if (count == 0) break;
        if (remaining < count)
            return start + remaining;
        remaining -= count;
    }

    // Fall through to the overflow B-tree
    if (!m_extents_tree) return fk::core::Error::InvalidParameter;

    // The overflow key's startBlock is the first block covered by each extent record
    // We need to find the record whose startBlock <= logical_block
    // Binary-search the overflow tree with decreasing startBlock guesses isn't trivial,
    // so we walk down using the inline extents' total-block count as the base.
    uint32_t inline_blocks = 0;
    for (int i = 0; i < kHFSPlusExtentDensity; ++i)
        inline_blocks += fk::algorithms::swap32(m_fork.extents[i].blockCount);

    // Look up the overflow record that covers (logical_block - inline_blocks)
    uint32_t overflow_start = inline_blocks; // first block in overflow
    // Try to look up the exact startBlock for this overflow chunk
    // HFS+ stores overflow extents in chunks of kHFSPlusExtentDensity blocks
    // The startBlock key in the overflow B-tree is the start of each 8-extent run
    // Round down to the nearest 8-extent boundary
    uint32_t chunk_size = 8; // each overflow record covers 8 extents
    (void)chunk_size;
    uint32_t overflow_block = (logical_block / 8) * 8;
    if (overflow_block < inline_blocks) overflow_block = inline_blocks;

    auto ov_res = btree_extents_lookup(*m_extents_tree, m_file_id, m_fork_type, overflow_block);
    if (ov_res.is_error()) return ov_res.error();

    const auto& exts = ov_res.value();
    uint32_t rem2 = logical_block - overflow_block;
    for (size_t j = 0; j < exts.size(); ++j) {
        uint32_t cnt = fk::algorithms::swap32(exts[j].blockCount);
        if (cnt == 0) break;
        if (rem2 < cnt)
            return fk::algorithms::swap32(exts[j].startBlock) + rem2;
        rem2 -= cnt;
    }

    (void)overflow_start;
    return fk::core::Error::InvalidParameter;
}

fk::core::Result<size_t, fk::core::Error>
HFSPlusForkReader::read(uint64_t offset, size_t size, uint8_t* buf) const
{
    if (offset >= m_logical_size) return (size_t)0;
    if (offset + size > m_logical_size)
        size = (size_t)(m_logical_size - offset);
    if (size == 0) return (size_t)0;

    uint32_t sector_size = m_device->sector_size().value();
    size_t   bytes_read  = 0;

    while (bytes_read < size) {
        uint64_t cur_offset  = offset + bytes_read;
        uint32_t logical_blk = (uint32_t)(cur_offset / m_block_size);
        uint32_t blk_offset  = (uint32_t)(cur_offset % m_block_size);

        auto phys_res = resolve_block(logical_blk);
        if (phys_res.is_error()) break;

        uint32_t phys_blk    = phys_res.value();
        uint64_t sector      = (uint64_t)m_first_sector
                             + (uint64_t)phys_blk * (m_block_size / sector_size);
        uint32_t chunk       = m_block_size - blk_offset;
        if (chunk > size - bytes_read) chunk = (uint32_t)(size - bytes_read);

        if (blk_offset == 0 && chunk == m_block_size) {
            // Aligned full-block read
            if (m_device->read_sectors(sector, m_block_size / sector_size, buf + bytes_read).is_error())
                break;
        } else {
            // Partial block — read full block then copy the slice
            fk::containers::Vector<uint8_t> tmp;
            tmp.resize(m_block_size);
            if (m_device->read_sectors(sector, m_block_size / sector_size, tmp.begin()).is_error())
                break;
            fk::memory::copy(buf + bytes_read, tmp.begin() + blk_offset, chunk);
        }
        bytes_read += chunk;
    }
    return bytes_read;
}

} // namespace fkernel
