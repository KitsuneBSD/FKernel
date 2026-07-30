#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

// ExFAT Boot Sector (first 512 bytes of volume)
struct ExfatBpb {
    uint8_t  jump_boot[3];           // 0
    char     oem_name[8];            // 3  "EXFAT   "
    uint8_t  must_be_zero[53];       // 11
    uint64_t partition_offset;       // 64
    uint64_t volume_length;          // 72
    uint32_t fat_offset;             // 80  sectors from volume start
    uint32_t fat_length;             // 84  sectors
    uint32_t cluster_heap_offset;    // 88  sectors from volume start
    uint32_t cluster_count;          // 92
    uint32_t root_cluster;           // 96  first cluster of root directory
    uint32_t volume_serial_number;   // 100
    uint16_t fs_revision;            // 104 0x0100 = 1.00
    uint16_t volume_flags;           // 106
    uint8_t  bytes_per_sector_shift; // 108 log2(bytes_per_sector), min 9
    uint8_t  sectors_per_cluster_shift; // 109 log2(sectors_per_cluster)
    uint8_t  number_of_fats;         // 110
    uint8_t  drive_select;           // 111
    uint8_t  percent_in_use;         // 112
} __attribute__((packed));

// Directory entry type codes
static constexpr uint8_t EXFAT_ET_END_OF_DIR    = 0x00;
static constexpr uint8_t EXFAT_ET_ALLOC_BITMAP  = 0x81;
static constexpr uint8_t EXFAT_ET_UPCASE_TABLE  = 0x82;
static constexpr uint8_t EXFAT_ET_VOLUME_LABEL  = 0x83;
static constexpr uint8_t EXFAT_ET_FILE          = 0x85;
static constexpr uint8_t EXFAT_ET_STREAM_EXT    = 0xC0;
static constexpr uint8_t EXFAT_ET_FILE_NAME     = 0xC1;
static constexpr uint8_t EXFAT_INUSE_BIT        = 0x80;

// File attributes
static constexpr uint16_t EXFAT_ATTR_READ_ONLY  = 0x0001;
static constexpr uint16_t EXFAT_ATTR_HIDDEN     = 0x0002;
static constexpr uint16_t EXFAT_ATTR_SYSTEM     = 0x0004;
static constexpr uint16_t EXFAT_ATTR_DIRECTORY  = 0x0010;
static constexpr uint16_t EXFAT_ATTR_ARCHIVE    = 0x0020;

// GeneralSecondaryFlags bits
static constexpr uint8_t EXFAT_SFLAG_ALLOC_POSSIBLE = 0x01;
static constexpr uint8_t EXFAT_SFLAG_NO_FAT_CHAIN   = 0x02;

// FAT chain terminators
static constexpr uint32_t EXFAT_CLUSTER_FREE    = 0x00000000;
static constexpr uint32_t EXFAT_CLUSTER_BAD     = 0xFFFFFFF7;
static constexpr uint32_t EXFAT_CLUSTER_EOC     = 0xFFFFFFFF;
static constexpr uint32_t EXFAT_CLUSTER_FIRST   = 2; // cluster numbers start at 2

// File entry (primary, type 0x85) — 32 bytes
struct ExfatFileEntry {
    uint8_t  entry_type;         // 0x85
    uint8_t  secondary_count;    // number of secondary entries following
    uint16_t set_checksum;
    uint16_t file_attributes;
    uint8_t  reserved1[2];
    uint32_t create_time;
    uint32_t last_modified_time;
    uint32_t last_accessed_time;
    uint8_t  create_10ms;
    uint8_t  last_modified_10ms;
    uint8_t  create_utc_offset;
    uint8_t  last_modified_utc_offset;
    uint8_t  last_accessed_utc_offset;
    uint8_t  reserved2[7];
} __attribute__((packed));

// Stream Extension (secondary, type 0xC0) — 32 bytes
struct ExfatStreamExtEntry {
    uint8_t  entry_type;             // 0xC0
    uint8_t  general_secondary_flags; // bit 1 = NoFatChain
    uint8_t  reserved1;
    uint8_t  name_length;            // characters in filename
    uint16_t name_hash;
    uint8_t  reserved2[2];
    uint64_t valid_data_length;
    uint8_t  reserved3[4];
    uint32_t first_cluster;
    uint64_t data_length;
} __attribute__((packed));

// File Name (secondary, type 0xC1) — 32 bytes, holds 15 UCS-2 LE chars
struct ExfatFileNameEntry {
    uint8_t  entry_type;             // 0xC1
    uint8_t  general_secondary_flags;
    uint16_t file_name[15];          // UCS-2 LE, up to 15 chars per entry
} __attribute__((packed));

// Allocation Bitmap (secondary, type 0x81) — 32 bytes
struct ExfatAllocBitmapEntry {
    uint8_t  entry_type;    // 0x81
    uint8_t  bitmap_flags;  // bit 0 = second FAT bitmap
    uint8_t  reserved[18];
    uint32_t first_cluster;
    uint64_t data_length;   // ceil(ClusterCount / 8) bytes
} __attribute__((packed));

} // namespace fkernel
