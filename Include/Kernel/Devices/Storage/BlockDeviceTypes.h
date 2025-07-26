#pragma once

#include <Kernel/Devices/Storage/BlockDevice.h>
#include <Kernel/Driver/Ata/AtaTypes.h>
#include <Kernel/FileSystem/VFS/VFSTypes.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace Device {

struct BlockDevice;

struct BlockDeviceNode {
    FileSystem::VNode vnode;
    BlockDevice* device;
    ChannelType channel;
    DriveType drive;
};

struct BlockDeviceOps {
    int (*open)(BlockDevice* dev);
    int (*close)(BlockDevice* dev);
    LibC::ssize_t (*read)(BlockDevice* dev, LibC::uint64_t block, void* buffer, LibC::size_t count);
    LibC::ssize_t (*write)(BlockDevice* dev, LibC::uint64_t block, void const* buffer, LibC::size_t count);
    int (*ioctl)(BlockDevice* dev, int request, void* arg);
};

inline BlockDevice* blockdevice_from_vnode(FileSystem::VNode* vnode)
{
    auto* node = container_of(vnode, BlockDeviceNode, vnode);
    return node->device;
}

inline BlockDeviceNode* blockdevice_node_from_device(BlockDevice* dev)
{
    return reinterpret_cast<BlockDeviceNode*>(dev);
}

}
