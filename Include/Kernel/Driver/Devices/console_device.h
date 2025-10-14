#pragma once

#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <Kernel/FileSystem/VirtualFS/vnode_type.h>
#include <LibC/stddef.h>

struct VNode;

struct ConsoleDevice
{
    static int open(VNode *vnode, int flags);
    static int close(VNode *vnode);
    static int write(VNode *vnode, const void *buffer, size_t size, size_t offset);
    static int read(VNode *vnode, void *buffer, size_t size, size_t offset);

    static VNodeOps ops;
};