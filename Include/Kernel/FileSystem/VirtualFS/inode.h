#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Represents an inode in a filesystem.
 *
 * The Inode structure holds metadata about a file or directory,
 * including its number, permissions, size, timestamps, and pointers
 * to its data blocks.
 */
struct Inode {
  uint64_t i_number;          ///< Unique inode number
  uint32_t permissions;       ///< File permissions (e.g., read/write/execute)
  uint64_t size;              ///< Size of the file in bytes
  uint64_t link_count;        ///< Number of hard links pointing to this inode
  uint64_t creation_time;     ///< Time of creation
  uint64_t modification_time; ///< Time of last modification
  uint64_t access_time;       ///< Time of last access
  uint64_t
      data_block_pointers[12]; ///< Direct pointers to data blocks (up to 12)

  /**
   * @brief Constructs a new inode with a given inode number.
   *
   * Initializes all metadata fields to default values.
   *
   * @param inode_num Unique inode number to assign
   */
  Inode(uint64_t inode_num)
      : i_number(inode_num), permissions(0), size(0), link_count(1),
        creation_time(0), modification_time(0), access_time(0) {
    for (size_t i = 0; i < 12; ++i)
      data_block_pointers[i] = 0;
  }
};
