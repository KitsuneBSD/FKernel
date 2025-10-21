#pragma once

#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <Kernel/FileSystem/fd.h>
#include <Kernel/Driver/Ata/AtaController.h>
#include <Kernel/Block/partition.h>

struct PartitionInfo {
    AtaDeviceInfo *device; // underlying physical device (heap-allocated)
    uint32_t lba_first;
    uint32_t sectors_count;
    uint8_t type;
};

struct PartitionBlockDevice {
    static int open(VNode *vnode, FileDescriptor *fd, int flags);
    static int close(VNode *vnode, FileDescriptor *fd);
    static int read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size, size_t offset);
    static int write(VNode *vnode, FileDescriptor *fd, const void *buffer, size_t size, size_t offset);

    static VNodeOps ops;
};
