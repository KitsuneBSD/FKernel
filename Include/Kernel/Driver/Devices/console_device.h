#pragma once

#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <Kernel/FileSystem/VirtualFS/vnode_type.h>
#include <Kernel/FileSystem/file_descriptor.h>
#include <LibFK/Types/types.h>

struct VNode;

/**
 * @brief Virtual file system interface for the kernel console device
 * 
 * Provides static methods compatible with the VFS to allow reading from
 * and writing to the kernel console.
 */
struct ConsoleDevice {
    /**
     * @brief Open the console device
     * 
     * @param vnode VNode representing the console
     * @param fd File descriptor
     * @param flags Open flags
     * @return 0 on success, negative error code on failure
     */
    static int open(VNode *vnode, FileDescriptor *fd, int flags);

    /**
     * @brief Close the console device
     * 
     * @param vnode VNode representing the console
     * @param fd File descriptor
     * @return 0 on success, negative error code on failure
     */
    static int close(VNode *vnode, FileDescriptor *fd);

    /**
     * @brief Write data to the console
     * 
     * @param vnode VNode representing the console
     * @param fd File descriptor
     * @param buffer Pointer to the data to write
     * @param size Number of bytes to write
     * @param offset Offset (ignored for console)
     * @return Number of bytes written on success, negative error code on failure
     */
    static int write(VNode *vnode, FileDescriptor *fd, const void *buffer,
                     size_t size, size_t offset);

    /**
     * @brief Read data from the console
     * 
     * @param vnode VNode representing the console
     * @param fd File descriptor
     * @param buffer Pointer to the buffer where data will be stored
     * @param size Number of bytes to read
     * @param offset Offset (ignored for console)
     * @return Number of bytes read on success, negative error code on failure
     */
    static int read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size,
                    size_t offset);

    /// Virtual node operations table for the console device
    static VNodeOps ops;
};
