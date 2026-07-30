#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

// UFS superblock constants
static constexpr uint32_t UFS_SBOFF    = 8192;   // byte offset of superblock from partition start
static constexpr uint32_t UFS_SBSIZE   = 1376;   // on-disk superblock size (bytes)
static constexpr uint32_t UFS1_MAGIC   = 0x011954;
static constexpr uint32_t UFS2_MAGIC   = 0x19540119;

// Byte offsets within the raw superblock buffer for the fields we need.
// Verified against FreeBSD sys/ufs/ffs/fs.h revision history.
static constexpr uint32_t UFS_OFF_IBLKNO  = 0x010; // int32: frag offset of inode table in CG
static constexpr uint32_t UFS_OFF_NCG     = 0x02C; // int32: number of cylinder groups
static constexpr uint32_t UFS_OFF_BSIZE   = 0x030; // int32: block size in bytes
static constexpr uint32_t UFS_OFF_FSIZE   = 0x034; // int32: fragment size in bytes
static constexpr uint32_t UFS_OFF_FRAG    = 0x038; // int32: fragments per block
static constexpr uint32_t UFS_OFF_FSBTODB = 0x064; // int32: frag-to-sector shift
static constexpr uint32_t UFS_OFF_NINDIR  = 0x074; // int32: block-addresses per indirect block
static constexpr uint32_t UFS_OFF_INOPB   = 0x078; // int32: inodes per block
static constexpr uint32_t UFS_OFF_IPG     = 0x0B8; // int32: inodes per group
static constexpr uint32_t UFS_OFF_FPG     = 0x0BC; // int32: frags per group
static constexpr uint32_t UFS_OFF_MAGIC   = 0x55C; // int32: magic number

// Mode bits
static constexpr uint16_t UFS_IFMT  = 0xF000;
static constexpr uint16_t UFS_IFREG = 0x8000;
static constexpr uint16_t UFS_IFDIR = 0x4000;
static constexpr uint16_t UFS_IFLNK = 0xA000;

// UFS1 on-disk inode — 128 bytes
// Layout per 4.4BSD / FreeBSD ufs/ufs/dinode.h
struct Ufs1Dinode {
    uint16_t di_mode;
    int16_t  di_nlink;
    uint16_t di_uid_old;
    uint16_t di_gid_old;
    uint64_t di_size;
    int32_t  di_atime;
    int32_t  di_atimensec;
    int32_t  di_mtime;
    int32_t  di_mtimensec;
    int32_t  di_ctime;
    int32_t  di_ctimensec;
    int32_t  di_db[12];  // direct block addresses
    int32_t  di_ib[3];   // indirect block addresses (single, double, triple)
    uint32_t di_flags;
    int32_t  di_blocks;
    int32_t  di_gen;
    uint32_t di_uid;
    uint32_t di_gid;
    uint64_t di_modrev;
} __attribute__((packed)); // 128 bytes

// UFS2 on-disk inode — 256 bytes
struct Ufs2Dinode {
    uint16_t di_mode;
    int16_t  di_nlink;
    uint32_t di_uid;
    uint32_t di_gid;
    uint32_t di_blksize;
    uint64_t di_size;
    uint64_t di_blocks;
    uint64_t di_atime;
    uint64_t di_mtime;
    uint64_t di_ctime;
    uint64_t di_birthtime;
    int32_t  di_mtimensec;
    int32_t  di_atimensec;
    int32_t  di_ctimensec;
    int32_t  di_birthnsec;
    int32_t  di_gen;
    uint32_t di_kernflags;
    uint32_t di_flags;
    int32_t  di_extsize;
    int64_t  di_extb[2];  // extended attribute blocks
    int64_t  di_db[12];   // direct block addresses (64-bit)
    int64_t  di_ib[3];    // indirect block addresses (64-bit)
    int64_t  di_spare[3];
} __attribute__((packed)); // 256 bytes

// Parsed mount parameters extracted from the raw superblock
struct UfsMountInfo {
    bool     is_ufs2;
    uint32_t bsize;         // block size in bytes
    uint32_t fsize;         // fragment size in bytes
    uint32_t frag;          // fragments per block
    uint32_t nindir;        // block addresses per indirect block
    uint32_t inopb;         // inodes per block
    uint32_t ncg;           // number of cylinder groups
    uint32_t ipg;           // inodes per group
    uint32_t fpg;           // frags per group
    uint32_t iblkno;        // frag offset of inode table within each CG
    uint32_t fsbtodb;       // frag-to-sector shift
};

} // namespace fkernel
