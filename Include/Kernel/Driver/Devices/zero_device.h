#pragma once

#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/DevFS/devfs.h>

struct VNode;

int dev_zero_read(VNode *vnode, void *buffer, size_t size, size_t offset);
int dev_zero_write(VNode *vnode, const void *buffer, size_t size, size_t offset);

extern VNodeOps zero_ops;