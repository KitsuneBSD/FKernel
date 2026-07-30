#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

static constexpr uint16_t MINIX1_MAGIC     = 0x137F; // V1, 14-char names
static constexpr uint16_t MINIX1_MAGIC30   = 0x138F; // V1, 30-char names
static constexpr uint32_t MINIX_BLOCK_SIZE = 1024;
static constexpr uint32_t MINIX_INODES_PER_BLOCK = 32; // 1024 / sizeof(MinixInode)
static constexpr uint32_t MINIX_DIRECT_ZONES     = 7;
static constexpr uint32_t MINIX_PTR_PER_BLOCK    = 512; // 1024 / 2 bytes per zone ptr

// On-disk superblock (at byte offset 1024, i.e. block 1)
struct MinixSuperBlock {
    uint16_t s_ninodes;
    uint16_t s_nzones;
    uint16_t s_imap_blocks;
    uint16_t s_zmap_blocks;
    uint16_t s_firstdatazone;
    uint16_t s_log_zone_size;
    uint32_t s_max_size;
    uint16_t s_magic;
    uint16_t s_state;
} __attribute__((packed));

// On-disk inode (32 bytes)
struct MinixInode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_mtime;
    uint8_t  i_gid;
    uint8_t  i_nlinks;
    uint16_t i_zone[9]; // [0..6] direct, [7] single-indirect, [8] double-indirect
} __attribute__((packed));

// On-disk directory entry, V1 (16 bytes, 14-char names)
struct MinixDirEntry {
    uint16_t inode;
    char     name[14];
} __attribute__((packed));

// On-disk directory entry, V1 30-char variant (32 bytes)
struct MinixDirEntry30 {
    uint16_t inode;
    char     name[30];
} __attribute__((packed));

} // namespace fkernel
