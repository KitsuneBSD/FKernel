#pragma once

#include <Kernel/Boot/Uefi/uefi_types.h>
#include <LibFK/Types/types.h>

namespace uefi {
namespace fat {

/**
 * @brief FAT filesystem types
 */
enum class FatType {
  FAT12,
  FAT16,
  FAT32,
  Unknown
};

/**
 * @brief FAT Boot Sector (BPB)
 */
struct BootSector {
  uint8_t jmp[3];
  char oem_name[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t num_fats;
  uint16_t root_entries;
  uint16_t total_sectors_16;
  uint8_t media_type;
  uint16_t fat_size_16;
  uint16_t sectors_per_track;
  uint16_t num_heads;
  uint32_t hidden_sectors;
  uint32_t total_sectors_32;
  
  // Extended BPB (FAT12/FAT16)
  uint8_t drive_number;
  uint8_t reserved;
  uint8_t boot_signature_old;
  uint32_t volume_id;
  char volume_label[11];
  char file_system_type[8];
  
  // FAT32 Extended BPB
  uint32_t fat_size_32;
  uint16_t ext_flags;
  uint16_t fs_version;
  uint32_t root_cluster;
  uint16_t fs_info;
  uint16_t backup_boot_sector;
  uint8_t reserved2[12];
  uint8_t boot_code[420];
  uint16_t boot_signature;
} __attribute__((packed));

/**
 * @brief FAT Directory Entry
 */
struct DirectoryEntry {
  char name[11];
  uint8_t attributes;
  uint8_t reserved;
  uint8_t creation_time_tenths;
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t access_date;
  uint16_t first_cluster_high;
  uint16_t modification_time;
  uint16_t modification_date;
  uint16_t first_cluster_low;
  uint32_t file_size;
} __attribute__((packed));

/**
 * @brief FAT filesystem context
 */
struct FatContext {
  EFI_BLOCK_IO_PROTOCOL *block_io;
  BootSector *boot_sector;
  FatType fat_type;
  uint32_t bytes_per_sector;
  uint32_t sectors_per_cluster;
  uint32_t root_dir_sectors;
  uint32_t first_data_sector;
  uint32_t first_fat_sector;
  uint32_t data_sector;
  uint32_t root_cluster; // For FAT32
  uint32_t root_dir_sector; // For FAT12/FAT16
};

/**
 * @brief Initialize FAT filesystem from block device
 * @param block_io Block I/O protocol
 * @param context FAT context to initialize
 * @return true on success, false on failure
 */
bool initialize(EFI_BLOCK_IO_PROTOCOL *block_io, FatContext &context);

/**
 * @brief Read a file from FAT filesystem
 * @param context FAT context
 * @param filename File name (8.3 format)
 * @param buffer Buffer to read into
 * @param buffer_size Size of buffer
 * @param bytes_read Output: number of bytes read
 * @return true on success, false on failure
 */
bool read_file(const FatContext &context,
               const char *filename,
               void *buffer,
               size_t buffer_size,
               size_t &bytes_read);

/**
 * @brief Find a file in the root directory
 * @param context FAT context
 * @param filename File name (8.3 format)
 * @param entry Output: directory entry if found
 * @return true if file found, false otherwise
 */
bool find_file(const FatContext &context,
               const char *filename,
               DirectoryEntry &entry);

/**
 * @brief Read a cluster from the data area
 * @param context FAT context
 * @param cluster Cluster number
 * @param buffer Buffer to read into
 * @return true on success, false on failure
 */
bool read_cluster(const FatContext &context,
                  uint32_t cluster,
                  void *buffer);

/**
 * @brief Get next cluster in FAT chain
 * @param context FAT context
 * @param cluster Current cluster number
 * @return Next cluster number, or 0xFFFFFFFF/0xFFFF/0xFFF for end of chain
 */
uint32_t get_next_cluster(const FatContext &context, uint32_t cluster);

} // namespace fat
} // namespace uefi
