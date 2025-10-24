#pragma once

#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/FileSystem/file_descriptor.h>

struct VNode;

int dev_zero_read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size, size_t offset);
int dev_zero_write(VNode *vnode, FileDescriptor *fd, const void *buffer, size_t size, size_t offset);

extern VNodeOps zero_ops;