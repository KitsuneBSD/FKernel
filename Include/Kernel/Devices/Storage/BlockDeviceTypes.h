#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace Device {

struct BlockDevice;

struct BlockDeviceOps {
    int (*open)(BlockDevice* dev);
    int (*close)(BlockDevice* dev);
    LibC::ssize_t (*read)(BlockDevice* dev, LibC::uint64_t block, void* buffer, LibC::size_t count);
    LibC::ssize_t (*write)(BlockDevice* dev, LibC::uint64_t block, void const* buffer, LibC::size_t count);
    int (*ioctl)(BlockDevice* dev, int request, void* arg);
};

}
