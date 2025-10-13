#pragma once

#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <Kernel/FileSystem/VirtualFS/vnode_type.h>

struct VNode;

int dev_null_read(VNode *vnode, void *buffer, size_t size, size_t offset);
int dev_null_write(VNode *vnode, const void *buffer, size_t size, size_t offset);

extern VNodeOps null_ops;