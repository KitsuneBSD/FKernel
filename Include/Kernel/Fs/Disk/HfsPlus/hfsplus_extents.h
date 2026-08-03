#pragma once

#include <Kernel/Fs/Disk/HfsPlus/hfsplus_vh.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_btree.h>
#include <Kernel/Driver/Storage/Interfaces/storage_device.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Core/result.h>
#include <LibFK/Types/types.h>

namespace fkernel {

// Fork I/O helper.  Reads bytes from a fork's data using both the 8 inline
// extents in HFSPlusForkData and, if needed, the extents overflow B-tree.
class HFSPlusForkReader {
public:
    HFSPlusForkReader() = default;

    void set(fk::RefPtr<StorageDevice> device,
             const BTreeFile* extents_tree,
             const HFSPlusForkData& fork,
             uint32_t block_size,
             uint32_t first_sector,
             uint32_t file_id,
             uint8_t  fork_type);

    // Read `size` bytes at `offset` into `buf`.
    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buf) const;

    uint64_t logical_size()  const { return m_logical_size; }
    uint32_t total_blocks()  const { return m_total_blocks; }

private:
    // Resolve a logical block index to a physical sector, checking overflow tree.
    fk::core::Result<uint32_t, fk::core::Error>
    resolve_block(uint32_t logical_block) const;

    fk::RefPtr<StorageDevice>          m_device;
    const BTreeFile*                   m_extents_tree{nullptr};
    HFSPlusForkData                    m_fork{};
    uint32_t                           m_block_size{0};
    uint32_t                           m_first_sector{0};
    uint32_t                           m_file_id{0};
    uint8_t                            m_fork_type{0};
    uint64_t                           m_logical_size{0};
    uint32_t                           m_total_blocks{0};
};

} // namespace fkernel
