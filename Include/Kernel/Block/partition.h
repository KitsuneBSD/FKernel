#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Represents a single partition table entry in the MBR (Master Boot
 * Record)
 */
struct PartitionEntry {
  uint8_t boot_flag; ///< Boot indicator: 0x80 = bootable, 0x00 = non-bootable
  uint8_t chs_first[3]; ///< CHS address of the first sector (Cylinder-Head-Sector)
  uint8_t type; ///< Partition type identifier (e.g., 0x83 = Linux, 0x07 = NTFS)
  uint8_t chs_last[3]; ///< CHS address of the last sector
  uint32_t lba_first;  ///< LBA (Logical Block Addressing) of the first sector
  uint32_t sectors_count; ///< Number of sectors in the partition
};
