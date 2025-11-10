#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/FileSystem/file_descriptor.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <LibFK/Memory/retain_ptr.h>

struct VNode;

/**
 * @brief Abstract base class for all block devices.
 *
 * Defines the common interface for block devices, allowing for polymorphic
 * handling of different storage types (e.g., ATA drives, partitions, RAM disks).
 */
class BlockDevice {
public:
    /**
     * @brief Reads data from the block device.
     *
     * @param vnode VNode representing this device.
     * @param fd File descriptor.
     * @param buffer Pointer to the buffer where data will be stored.
     * @param size Number of bytes to read.
     * @param offset Offset in the device to start reading from.
     * @return Number of bytes read on success, negative error code on failure.
     */
    virtual int read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size, size_t offset) = 0;

    /**
     * @brief Writes data to the block device.
     *
     * @param vnode VNode representing this device.
     * @param fd File descriptor.
     * @param buffer Pointer to the data to write.
     * @param size Number of bytes to write.
     * @param offset Offset in the device to start writing to.
     * @return Number of bytes written on success, negative error code on failure.
     */
    virtual int write(VNode *vnode, FileDescriptor *fd, const void *buffer, size_t size, size_t offset) = 0;

    /**
     * @brief Opens the block device.
     *
     * @param vnode VNode representing this device.
     * @param fd File descriptor.
     * @param flags Open flags.
     * @return 0 on success, negative error code on failure.
     */
    virtual int open(VNode *vnode, FileDescriptor *fd, int flags) = 0;

    /**
     * @brief Closes the block device.
     *
     * @param vnode VNode representing this device.
     * @param fd File descriptor.
     * @return 0 on success, negative error code on failure.
     */
    virtual int close(VNode *vnode, FileDescriptor *fd) = 0;

    virtual ~BlockDevice() = default;

    // Reference counting for RetainPtr
    void retain() { m_ref_count++; }
    void release() {
        if (--m_ref_count == 0) {
            delete this;
        }
    }

protected:
    BlockDevice() : m_ref_count(0) {}

private:
    size_t m_ref_count;
};

// Global VNodeOps for block devices, which will dispatch to the BlockDevice interface
extern VNodeOps g_block_device_ops;
