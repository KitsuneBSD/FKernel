#pragma once

#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <Kernel/FileSystem/file_descriptor.h>

/**
 * @brief Block device interface for ATA devices
 *
 * Provides static methods compatible with the Virtual File System (VFS)
 * to perform operations on a physical ATA device, including open, close,
 * read, and write.
 */
struct AtaBlockDevice {
  /**
   * @brief Open an ATA device
   *
   * @param vnode VNode representing this device
   * @param fd File descriptor
   * @param flags Open flags
   * @return 0 on success, negative error code on failure
   */
  static int open(VNode *vnode, FileDescriptor *fd, int flags);

  /**
   * @brief Close an ATA device
   *
   * @param vnode VNode representing this device
   * @param fd File descriptor
   * @return 0 on success, negative error code on failure
   */
  static int close(VNode *vnode, FileDescriptor *fd);

  /**
   * @brief Read data from the ATA device
   *
   * @param vnode VNode representing this device
   * @param fd File descriptor
   * @param buffer Pointer to the buffer where data will be stored
   * @param size Number of bytes to read
   * @param offset Offset in the device to start reading from
   * @return Number of bytes read on success, negative error code on failure
   */
  static int read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size,
                  size_t offset);

  /**
   * @brief Write data to the ATA device
   *
   * @param vnode VNode representing this device
   * @param fd File descriptor
   * @param buffer Pointer to the data to write
   * @param size Number of bytes to write
   * @param offset Offset in the device to start writing to
   * @return Number of bytes written on success, negative error code on failure
   */
  static int write(VNode *vnode, FileDescriptor *fd, const void *buffer,
                   size_t size, size_t offset);

  /// Virtual node operations table for this block device
  static VNodeOps ops;
};
