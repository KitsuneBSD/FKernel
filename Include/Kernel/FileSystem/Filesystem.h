#pragma once

#include <LibFK/Container/static_vector.h>
#include <LibFK/Memory/retain_ptr.h>
#include <Kernel/FileSystem/VirtualFS/VNode.h>
#include <Kernel/FileSystem/VirtualFS/VNodeOps.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibC/string.h>

struct BlockDevice;

/**
 * @brief Interface for a filesystem driver.
 *
 * Each filesystem driver is responsible for interpreting a block device
 * and creating the corresponding VNode tree.
 */
struct FileSystemDriver
{
    const char *m_name; ///< Name of Filesystem

    /**
     * @brief Initialize the filesystem driver (optional).
     * @return 0 on success, negative on error
     */
    int (*init)();

    /**
     * @brief Mount a device and return the root vnode.
     * @param device Pointer to the block device to mount
     * @return Root VNode of the mounted filesystem
     */
    RetainPtr<VNode> (*mount)(BlockDevice *device);
};