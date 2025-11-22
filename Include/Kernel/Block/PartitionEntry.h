#pragma once

#include <LibFK/Types/types.h>

enum class PartitionType : uint8_t {
  Unknown = 0x00,
  FAT12 = 0x01,
  FAT16 = 0x04,
  Extended = 0x05,
  FAT32 = 0x0B,
  LINUX_SWAP = 0x82,
  LINUX_FILESYSTEM = 0x83,
  GPT_PROTECTIVE_MBR = 0xEE,
  EFI_SYSTEM = 0xEF,
};

/**
 * @brief Represents a single partition table entry in the MBR (Master Boot
 * Record) or GPT
 */
struct PartitionEntry {
  PartitionType type; ///< Type of the partition (e.g., FAT, NTFS, Linux)
  uint32_t lba_start; ///< Starting Logical Block Address
  uint32_t lba_count; ///< Number of sectors in the partition
  bool is_bootable;   ///< True if this partition is bootable
  bool has_chs;       ///< True if CHS information is valid
  uint8_t chs_start[3]; ///< CHS start address (Cylinder, Head, Sector)
  uint8_t chs_end[3];   ///< CHS end address (Cylinder, Head, Sector)
} __attribute__((packed));
