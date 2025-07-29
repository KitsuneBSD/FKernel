#pragma once

#include <Kernel/Devices/Storage/BlockDevice.h>
#include <Kernel/Devices/Storage/BlockDeviceTypes.h>

extern Device::BlockDeviceOps AtaBlockDeviceOps;

int ata_open(Device::BlockDevice* dev);
int ata_close(Device::BlockDevice* dev);
LibC::ssize_t ata_read(Device::BlockDevice* dev, LibC::uint64_t block, void* buf, LibC::size_t count);
LibC::ssize_t ata_write(Device::BlockDevice* dev, LibC::uint64_t block, void const* buf, LibC::size_t count);
int ata_ioctl(Device::BlockDevice* dev, int request, void* arg);
