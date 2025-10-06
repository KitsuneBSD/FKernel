#pragma once

#include <Kernel/FileSystem/VirtualFS/VNode.h>

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/Memory/retain_ptr.h>

struct VNode;

/**
 * @brief Interface for VNode operations.
 *
 * This structure defines the operations that can be performed
 * on a vnode. Each filesystem or device can provide its own
 * implementation of these functions.
 */
struct VNodeOps
{
    /**
     * @brief Read data from a vnode.
     * @param node Pointer to vnode
     * @param buffer Destination buffer
     * @param size Number of bytes to read
     * @param offset Offset from the beginning
     * @return Number of bytes read, or negative on error
     */
    ssize_t (*read)(VNode *node, void *buffer, size_t size, size_t offset);

    /**
     * @brief Write data to a vnode.
     * @param node Pointer to vnode
     * @param buffer Source buffer
     * @param size Number of bytes to write
     * @param offset Offset from the beginning
     * @return Number of bytes written, or negative on error
     */
    ssize_t (*write)(VNode *node, const void *buffer, size_t size, size_t offset);

    /**
     * @brief Open a vnode.
     * @param node Pointer to vnode
     * @return 0 on success, negative on error
     */
    int (*open)(VNode *node);

    /**
     * @brief Close a vnode.
     * @param node Pointer to vnode
     * @return 0 on success, negative on error
     */
    int (*close)(VNode *node);

    /**
     * @brief Lookup a child vnode by name (for directories).
     * @param node Pointer to directory vnode
     * @param name Name of the child
     * @return Pointer to the child vnode, or nullptr if not found
     */
    VNode *(*lookup)(VNode *node, const char *name);

    /**
     * @brief Create a new child vnode (for directories).
     * @param node Pointer to parent directory vnode
     * @param name Name of new child
     * @param type Type of new vnode
     * @return Pointer to the new vnode, or nullptr on error
     */
    VNode *(*create)(VNode *node, const char *name, VNodeType type);

    /**
     * @brief Remove a child vnode (for directories).
     * @param node Pointer to parent directory vnode
     * @param child Pointer to child vnode to remove
     * @return 0 on success, negative on error
     */
    int (*remove)(VNode *node, VNode *child);
};