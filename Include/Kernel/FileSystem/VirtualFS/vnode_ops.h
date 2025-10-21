#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/Memory/retain_ptr.h>

#include <Kernel/FileSystem/VirtualFS/vnode_type.h>

// Forward declare FileDescriptor to avoid circular includes
struct FileDescriptor;

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
    int (*read)(VNode *Vnode, FileDescriptor *fd, void *buffer, size_t size, size_t offset);
    int (*write)(VNode *Vnode, FileDescriptor *fd, const void *buffer, size_t size, size_t offset);
    int (*open)(VNode *Vnode, FileDescriptor *fd, int flags);
    int (*close)(VNode *Vnode, FileDescriptor *fd);
    int (*lookup)(VNode *Vnode, FileDescriptor *fd, const char *name, RetainPtr<VNode> &result);
    int (*create)(VNode *Vnode, FileDescriptor *fd, const char *name, VNodeType type, RetainPtr<VNode> &result);
    int (*readdir)(VNode *Vnode, FileDescriptor *fd, void *dirent_buffer, size_t max_entries);
    int (*unlink)(VNode *vnode, FileDescriptor *fd, const char *name);
};