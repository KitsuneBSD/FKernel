#pragma once

#include <Kernel/FileSystem/VirtualFS/VNode.h>
#include <Kernel/FileSystem/VirtualFS/VNodeOps.h>
#include <Kernel/FileSystem/FileDescriptor.h>

/**
 * @brief Represents an open file in RamFS.
 */
struct RamFSFile
{
    RetainPtr<VNode> vnode; ///< Associated VNode
    size_t offset;          ///< Current read/write offset
};

/**
 * @brief RamFS VNode operations.
 *
 * Provides function pointers for standard filesystem operations
 * such as read, write, lookup, create, and remove.
 */
extern VNodeOps ramfs_ops;

/**
 * @brief Initializes the RamFS root directory.
 *
 * @return RetainPtr to the root VNode of the RamFS.
 */
RetainPtr<VNode> ramfs_init();

/* ==========================
   RamFS VNode operations
   ========================== */

/**
 * @brief Reads data from a RamFS VNode.
 *
 * @param node Pointer to the VNode to read from.
 * @param buffer Destination buffer.
 * @param size Number of bytes to read.
 * @param offset Offset from the start of the file.
 * @return Number of bytes read.
 */
ssize_t ramfs_read(VNode *node, void *buffer, size_t size, size_t offset);

/**
 * @brief Writes data to a RamFS VNode.
 *
 * @param node Pointer to the VNode to write to.
 * @param buffer Source buffer containing data to write.
 * @param size Number of bytes to write.
 * @param offset Offset from the start of the file.
 * @return Number of bytes written.
 */
ssize_t ramfs_write(VNode *node, const void *buffer, size_t size, size_t offset);

/**
 * @brief Opens a RamFS VNode.
 *
 * @param node Pointer to the VNode to open.
 * @return 0 on success, negative on error.
 */
int ramfs_open(VNode *node);

/**
 * @brief Closes a RamFS VNode.
 *
 * @param node Pointer to the VNode to close.
 * @return 0 on success, negative on error.
 */
int ramfs_close(VNode *node);

/**
 * @brief Looks up a child VNode by name in a directory VNode.
 *
 * @param node Pointer to the directory VNode.
 * @param name Name of the child to find.
 * @return Pointer to the child VNode if found, nullptr otherwise.
 */
VNode *ramfs_lookup(VNode *node, const char *name);

/**
 * @brief Creates a new child VNode under a directory VNode.
 *
 * @param node Pointer to the parent directory VNode.
 * @param name Name of the new child VNode.
 * @param type Type of the new VNode (File, Directory, etc.).
 * @return Pointer to the newly created child VNode, or nullptr on failure.
 */
VNode *ramfs_create(VNode *node, const char *name, VNodeType type);

/**
 * @brief Removes a child VNode from a directory.
 *
 * @param node Pointer to the parent directory VNode.
 * @param child Pointer to the child VNode to remove.
 * @return 0 on success, negative on error.
 */
int ramfs_remove(VNode *node, VNode *child);
