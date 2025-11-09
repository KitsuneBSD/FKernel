#pragma once

#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <Kernel/FileSystem/VirtualFS/vnode_type.h>
#include <Kernel/FileSystem/file_descriptor.h>

struct VNode;

/**
 * @brief Read from /dev/null
 *
 * Always returns 0 bytes read.
 *
 * @param vnode VNode representing /dev/null
 * @param fd File descriptor
 * @param buffer Destination buffer (ignored)
 * @param size Number of bytes requested
 * @param offset File offset (ignored)
 * @return Always returns 0
 */
int dev_null_read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size,
                  size_t offset);

/**
 * @brief Write to /dev/null
 *
 * Discards all data written and returns the number of bytes "written".
 *
 * @param vnode VNode representing /dev/null
 * @param fd File descriptor
 * @param buffer Source buffer
 * @param size Number of bytes to write
 * @param offset File offset (ignored)
 * @return Number of bytes written (size)
 */
int dev_null_write(VNode *vnode, FileDescriptor *fd, const void *buffer,
                   size_t size, size_t offset);

/**
 * @brief Virtual node operations for /dev/null
 */
extern VNodeOps null_ops;
