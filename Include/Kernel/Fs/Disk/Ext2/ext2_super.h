#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

static constexpr uint16_t EXT2_SUPER_MAGIC = 0xEF53;
static constexpr uint32_t EXT2_ROOT_INO    = 2;
static constexpr uint32_t EXT2_FIRST_INO   = 11;
static constexpr uint32_t EXT2_NDIR_BLOCKS = 12;

// Incompatible features that prevent mounting
static constexpr uint32_t EXT2_INCOMPAT_RECOVER    = 0x0004;
static constexpr uint32_t EXT2_INCOMPAT_JOURNAL_DEV= 0x0008;
static constexpr uint32_t EXT2_INCOMPAT_UNSUPPORTED = ~0x0002u; // 0x0002 = filetype OK

// Short symlink threshold: target inline in i_block[] if i_size <= this
static constexpr uint32_t EXT2_SYMLINK_INLINE_MAX  = 60;

// i_flags
static constexpr uint32_t EXT2_EXTENTS_FL = 0x00080000; // ext4 extents (reject)

// Directory file_type
static constexpr uint8_t EXT2_FT_UNKNOWN  = 0;
static constexpr uint8_t EXT2_FT_REG_FILE = 1;
static constexpr uint8_t EXT2_FT_DIR      = 2;
static constexpr uint8_t EXT2_FT_SYMLINK  = 7;

struct Ext2Superblock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block; // 1 for 1KB blocks, 0 for larger
    uint32_t s_log_block_size;   // block_size = 1024 << s_log_block_size
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    // Dynamic rev fields (rev_level >= 1)
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];
    char     s_volume_name[16];
    char     s_last_mounted[64];
    uint32_t s_algo_bitmap;
} __attribute__((packed));

struct Ext2BlockGroupDesc {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint8_t  bg_reserved[12];
} __attribute__((packed));

struct Ext2Inode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks; // in 512-byte units
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15]; // 12 direct + single + double + triple indirect
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t  i_osd2[12];
} __attribute__((packed));

// Variable-length directory entry header (name follows immediately)
struct Ext2DirEntry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    // char name[name_len] follows
} __attribute__((packed));

} // namespace fkernel
