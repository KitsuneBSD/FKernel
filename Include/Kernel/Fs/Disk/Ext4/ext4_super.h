#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

// ext4 incompatible feature flags (in s_feature_incompat)
static constexpr uint32_t EXT4_INCOMPAT_EXTENTS  = 0x0040; // extent tree
static constexpr uint32_t EXT4_INCOMPAT_FLEX_BG  = 0x0200; // flexible block groups
static constexpr uint32_t EXT4_INCOMPAT_64BIT     = 0x0080; // 64-bit block numbers

// ext4 inode flags
static constexpr uint32_t EXT4_EXTENTS_FL = 0x00080000; // inode uses extent tree
static constexpr uint32_t EXT4_INDEX_FL   = 0x00001000; // directory uses HTree index

// Extent tree magic in inode.i_block (LE)
static constexpr uint16_t EXT4_EH_MAGIC  = 0xF30A;

// Extent header (12 bytes) — stored at i_block[0] or in an index block
struct Ext4ExtentHeader {
    uint16_t eh_magic;      // EXT4_EH_MAGIC
    uint16_t eh_entries;    // number of valid extents/indexes
    uint16_t eh_max;        // max capacity of this node
    uint16_t eh_depth;      // depth (0 = leaf node)
    uint32_t eh_generation;
} __attribute__((packed));

// Leaf node entry (12 bytes)
struct Ext4Extent {
    uint32_t ee_block;      // first logical block this extent covers
    uint16_t ee_len;        // block count; bit 15 = uninitialized
    uint16_t ee_start_hi;   // high 16 bits of physical block number
    uint32_t ee_start_lo;   // low 32 bits of physical block number
} __attribute__((packed));

// Internal (index) node entry (12 bytes)
struct Ext4ExtentIdx {
    uint32_t ei_block;      // first logical block this index covers
    uint32_t ei_leaf_lo;    // low 32 bits of pointer to next-level block
    uint16_t ei_leaf_hi;    // high 16 bits of pointer
    uint16_t ei_unused;
} __attribute__((packed));

} // namespace fkernel
