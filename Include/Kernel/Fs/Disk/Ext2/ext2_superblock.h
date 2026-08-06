#pragma once
#include <LibFK/Types/types.h>
namespace fkernel {
static constexpr uint16_t EXT2_SUPER_MAGIC           = 0xEF53;
static constexpr uint32_t EXT2_ROOT_INO              = 2;
static constexpr uint32_t EXT2_FIRST_INO             = 11;
static constexpr uint32_t EXT2_NDIR_BLOCKS           = 12;
static constexpr uint32_t EXT2_INCOMPAT_RECOVER      = 0x0004;
static constexpr uint32_t EXT2_INCOMPAT_JOURNAL_DEV  = 0x0008;
static constexpr uint32_t EXT2_INCOMPAT_UNSUPPORTED  = ~0x0002u;
static constexpr uint32_t EXT2_SYMLINK_INLINE_MAX    = 60;
static constexpr uint32_t EXT2_EXTENTS_FL            = 0x00080000;
static constexpr uint8_t  EXT2_FT_UNKNOWN            = 0;
static constexpr uint8_t  EXT2_FT_REG_FILE           = 1;
static constexpr uint8_t  EXT2_FT_DIR                = 2;
static constexpr uint8_t  EXT2_FT_SYMLINK            = 7;
struct Ext2Superblock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
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
} // namespace fkernel
