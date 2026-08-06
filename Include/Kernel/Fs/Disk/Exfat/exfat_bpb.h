#pragma once

#include <Kernel/Fs/Disk/Exfat/exfat_alloc_bitmap_entry.h>
#include <Kernel/Fs/Disk/Exfat/exfat_file_entry.h>
#include <Kernel/Fs/Disk/Exfat/exfat_file_name_entry.h>
#include <Kernel/Fs/Disk/Exfat/exfat_stream_ext_entry.h>
#include <LibFK/Types/types.h>

namespace fkernel {

static constexpr uint8_t  EXFAT_ET_END_OF_DIR   = 0x00;
static constexpr uint8_t  EXFAT_ET_ALLOC_BITMAP = 0x81;
static constexpr uint8_t  EXFAT_ET_UPCASE_TABLE = 0x82;
static constexpr uint8_t  EXFAT_ET_VOLUME_LABEL = 0x83;
static constexpr uint8_t  EXFAT_ET_FILE         = 0x85;
static constexpr uint8_t  EXFAT_ET_STREAM_EXT   = 0xC0;
static constexpr uint8_t  EXFAT_ET_FILE_NAME    = 0xC1;
static constexpr uint8_t  EXFAT_INUSE_BIT       = 0x80;
static constexpr uint16_t EXFAT_ATTR_READ_ONLY  = 0x0001;
static constexpr uint16_t EXFAT_ATTR_HIDDEN     = 0x0002;
static constexpr uint16_t EXFAT_ATTR_SYSTEM     = 0x0004;
static constexpr uint16_t EXFAT_ATTR_DIRECTORY  = 0x0010;
static constexpr uint16_t EXFAT_ATTR_ARCHIVE    = 0x0020;
static constexpr uint8_t  EXFAT_SFLAG_ALLOC_POSSIBLE = 0x01;
static constexpr uint8_t  EXFAT_SFLAG_NO_FAT_CHAIN   = 0x02;
static constexpr uint32_t EXFAT_CLUSTER_FREE    = 0x00000000;
static constexpr uint32_t EXFAT_CLUSTER_BAD     = 0xFFFFFFF7;
static constexpr uint32_t EXFAT_CLUSTER_EOC     = 0xFFFFFFFF;
static constexpr uint32_t EXFAT_CLUSTER_FIRST   = 2;

struct ExfatBpb {
    uint8_t  jump_boot[3];
    char     oem_name[8];
    uint8_t  must_be_zero[53];
    uint64_t partition_offset;
    uint64_t volume_length;
    uint32_t fat_offset;
    uint32_t fat_length;
    uint32_t cluster_heap_offset;
    uint32_t cluster_count;
    uint32_t root_cluster;
    uint32_t volume_serial_number;
    uint16_t fs_revision;
    uint16_t volume_flags;
    uint8_t  bytes_per_sector_shift;
    uint8_t  sectors_per_cluster_shift;
    uint8_t  number_of_fats;
    uint8_t  drive_select;
    uint8_t  percent_in_use;
} __attribute__((packed));

} // namespace fkernel
