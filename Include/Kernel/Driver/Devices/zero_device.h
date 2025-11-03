#pragma once

#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/file_descriptor.h>

struct VNode;

/**
 * @brief Read from /dev/zero
 *
 * Fills the provided buffer with zeros.
 *
 * @param vnode VNode representing /dev/zero
 * @param fd File descriptor
 * @param buffer Destination buffer
 * @param size Number of bytes to read
 * @param offset File offset (ignored)
 * @return Number of bytes filled
 */
int dev_zero_read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size,
                  size_t offset);

/**
 * @brief Write to /dev/zero
 *
 * Discards all written data and returns the number of bytes "written".
 *
 * @param vnode VNode representing /dev/zero
 * @param fd File descriptor
 * @param buffer Source buffer (ignored)
 * @param size Number of bytes to write
 * @param offset File offset (ignored)
 * @return Number of bytes "written" (size)
 */
int dev_zero_write(VNode *vnode, FileDescriptor *fd, const void *buffer,
                   size_t size, size_t offset);

/**
 * @brief Virtual node operations for /dev/zero
 */
extern VNodeOps zero_ops;
