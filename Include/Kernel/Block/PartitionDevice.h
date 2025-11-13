#pragma once

#include <Kernel/Block/BlockDevice.h>
#include <Kernel/Block/PartitionEntry.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <Kernel/FileSystem/file_descriptor.h>
#include <LibFK/Memory/retain_ptr.h> // Include RetainPtr

/**
 * @brief Represents a logical partition on a physical ATA device
 */
struct PartitionInfo {
  RetainPtr<BlockDevice> device; ///< Pointer to the underlying physical ATA
                                 ///< device (heap-allocated)
  uint32_t lba_first;            ///< LBA of the first sector of the partition
  uint32_t sectors_count;        ///< Total number of sectors in the partition
  uint8_t type; ///< Partition type identifier (e.g., MBR partition type)
};

/**
 * @brief Block device interface for partitions
 *
 * Provides static methods compatible with the Virtual File System (VFS)
 * to perform operations on a logical partition, including open, close,
 * read, and write.
 */
class PartitionBlockDevice : public BlockDevice {
public:
  /**
   * @brief Open a partition
   *
   * @param vnode VNode representing this partition
   * @param fd File descriptor
   * @param flags Open flags
   * @return 0 on success, negative error code on failure
   */
  int open(VNode *vnode, FileDescriptor *fd, int flags) override;

  /**
   * @brief Close a partition
   *
   * @param vnode VNode representing this partition
   * @param fd File descriptor
   * @return 0 on success, negative error code on failure
   */
  int close(VNode *vnode, FileDescriptor *fd) override;

  /**
   * @brief Read data from the partition
   *
   * @param vnode VNode representing this partition
   * @param fd File descriptor
   * @param buffer Pointer to the buffer where data will be stored.
   * @param size Number of bytes to read.
   * @param offset Offset in the partition to start reading from.
   * @return Number of bytes read on success, negative error code on failure.
   */
  int read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size,
           size_t offset) override;

  /**
   * @brief Write data to the partition
   *
   * @param vnode VNode representing this partition
   * @param fd File descriptor
   * @param buffer Pointer to the data to write.
   * @param size Number of bytes to write.
   * @param offset Offset in the partition to start writing to.
   * @return Number of bytes written on success, negative error code on failure.
   */
  int write(VNode *vnode, FileDescriptor *fd, const void *buffer, size_t size,
            size_t offset) override;
};
