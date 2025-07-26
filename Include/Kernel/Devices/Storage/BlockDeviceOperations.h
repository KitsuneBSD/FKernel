#pragma once

#include <Kernel/Devices/Storage/BlockDevice.hpp>

namespace Device {

struct BlockDevice;

int blockdevice_open(Device::BlockDevice* dev);
int blockdevice_close(Device::BlockDevice* dev);
LibC::ssize_t blockdevice_read(BlockDevice* dev, LibC::uint64_t block, void* buffer, LibC::size_t count);
LibC::ssize_t blockdevice_write(BlockDevice* dev, LibC::uint64_t block, void const* buffer, LibC::size_t count);
int blockdevice_ioctl(BlockDevice* dev, int request, void* arg);

}
