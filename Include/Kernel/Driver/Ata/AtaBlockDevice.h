#pragma once

#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/FileSystem/file_descriptor.h>

struct AtaBlockDevice
{
    static int open(VNode *vnode, FileDescriptor *fd, int flags);
    static int close(VNode *vnode, FileDescriptor *fd);
    static int read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size, size_t offset);
    static int write(VNode *vnode, FileDescriptor *fd, const void *buffer, size_t size, size_t offset);

    static VNodeOps ops;
};
