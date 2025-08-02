#pragma once

#include <Kernel/Devices/Storage/BlockDevice.h>

namespace Device {

int blockdevice_open(BlockDevice* dev);
int blockdevice_close(BlockDevice* dev);
LibC::ssize_t blockdevice_read(BlockDevice* dev, LibC::uint64_t block, void* buffer, LibC::size_t count);
LibC::ssize_t blockdevice_write(BlockDevice* dev, LibC::uint64_t block, void const* buffer, LibC::size_t count);
int blockdevice_ioctl(BlockDevice* dev, int request, void* arg);

}
