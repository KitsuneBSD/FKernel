#pragma once

#include <LibFK/Types/types.h>

namespace fkernel::fs::fat {

// @brief FAT Boot Sector structure (common for FAT12/16/32)
struct [[gnu::packed]] FatBootSector {
  uint8_t jump_instruction[3];    ///< Jump instruction to boot code
  char oem_id[8];                 ///< OEM ID
  uint16_t bytes_per_sector;      ///< Bytes per sector (usually 512)
  uint8_t sectors_per_cluster;    ///< Sectors per cluster (power of 2)
  uint16_t reserved_sectors;      ///< Number of reserved sectors (usually 1 for FAT12/16, 32 for FAT32)
  uint8_t fat_count;              ///< Number of File Allocation Tables (usually 2)
  uint16_t root_dir_entries;      ///< Number of root directory entries (0 for FAT32)
  uint16_t total_sectors_16;      ///< Total sectors in filesystem (if less than 32MB)
  uint8_t media_type;             ///< Media descriptor type
  uint16_t sectors_per_fat_16;    ///< Sectors per FAT (for FAT12/16)
  uint16_t sectors_per_track;     ///< Sectors per track
  uint16_t head_count;            ///< Number of heads
  uint32_t hidden_sectors;        ///< Number of hidden sectors
  uint32_t total_sectors_32;      ///< Total sectors in filesystem (if total_sectors_16 is 0)

  // FAT12/16 specific fields
  uint8_t drive_number;           ///< Drive number
  uint8_t reserved_1;             ///< Reserved
  uint8_t boot_signature;         ///< Extended boot signature (0x29)
  uint32_t volume_id;             ///< Volume ID
  char volume_label[11];          ///< Volume label
  char fs_type_16[8];             ///< Filesystem type (e.g., "FAT12   ", "FAT16   ")

  // FAT32 specific fields
  uint32_t sectors_per_fat_32;    ///< Sectors per FAT (for FAT32)
  uint16_t ext_flags;             ///< Extended flags
  uint16_t fs_version;            ///< Filesystem version
  uint32_t root_cluster;          ///< Root directory entry cluster (usually 2)
  uint16_t fs_info_sector;        ///< Filesystem info sector
  uint16_t backup_boot_sector;    ///< Backup boot sector
  uint8_t reserved_2[12];         ///< Reserved
  uint8_t drive_number_32;        ///< Drive number
  uint8_t reserved_3;             ///< Reserved
  uint8_t boot_signature_32;      ///< Extended boot signature (0x29)
  uint32_t volume_id_32;          ///< Volume ID
  char volume_label_32[11];       ///< Volume label
  char fs_type_32[8];             ///< Filesystem type (e.g., "FAT32   ")

  uint8_t boot_code[420];         ///< Boot code
  uint16_t boot_sector_signature; ///< Boot sector signature (0xAA55)
};

// @brief FAT Directory Entry structure
struct [[gnu::packed]] FatDirEntry {
  char filename[8];               ///< 8-byte filename
  char extension[3];              ///< 3-byte extension
  uint8_t attributes;             ///< File attributes
  uint8_t reserved;               ///< Reserved
  uint8_t creation_time_tenths;   ///< Creation time (tenths of a second)
  uint16_t creation_time;         ///< Creation time
  uint16_t creation_date;         ///< Creation date
  uint16_t last_access_date;      ///< Last access date
  uint16_t first_cluster_high;    ///< High 16-bits of first cluster (for FAT32)
  uint16_t last_modification_time; ///< Last modification time
  uint16_t last_modification_date; ///< Last modification date
  uint16_t first_cluster_low;     ///< Low 16-bits of first cluster
  uint32_t file_size;             ///< File size in bytes
};

// @brief FAT Long File Name (LFN) Directory Entry structure
struct [[gnu::packed]] FatLfnDirEntry {
  uint8_t sequence_number;        ///< Sequence number and allocation status
  uint16_t name_part_1[5];        ///< First 5 characters of LFN
  uint8_t attributes;             ///< Attributes (must be 0x0F)
  uint8_t type;                   ///< Type (always 0x00)
  uint8_t checksum;               ///< Checksum of short filename
  uint16_t name_part_2[6];        ///< Next 6 characters of LFN
  uint16_t first_cluster_zero;    ///< Must be 0
  uint16_t name_part_3[2];        ///< Last 2 characters of LFN
};

// @brief FAT File Attributes
enum FatFileAttributes : uint8_t {
  READ_ONLY   = 0x01,
  HIDDEN      = 0x02,
  SYSTEM      = 0x04,
  VOLUME_ID   = 0x08,
  DIRECTORY   = 0x10,
  ARCHIVE     = 0x20,
  LFN         = READ_ONLY | HIDDEN | SYSTEM | VOLUME_ID // 0x0F
};

} // namespace fkernel::fs::fat