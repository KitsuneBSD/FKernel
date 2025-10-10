#pragma once

#include <Kernel/FileSystem/VirtualFS/vnode.h>

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
    int (*read)(VNode *Vnode, void *buffer, size_t size, size_t offset);
    int (*write)(VNode *Vnode, const void *buffer, size_t size, size_t offset);
    int (*open)(VNode *Vnode, int flags);
    int (*close)(VNode *Vnode);
    int (*lookup)(VNode *Vnode, const char *name, RetainPtr<VNode> &result);
    int (*create)(VNode *Vnode, const char *name, VNodeType type, RetainPtr<VNode> &result);
    int (*readdir)(VNode *Vnode, void *dirent_buffer, size_t max_entries);
    int (*unlink)(VNode *vnode, const char *name);
};