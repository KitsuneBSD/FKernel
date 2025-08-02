#include <Kernel/Devices/Storage/BlockDevice.h>
#include <Kernel/Devices/Storage/BlockDeviceOperations.h>
#include <Kernel/Devices/Storage/BlockDeviceTypes.h>

namespace Device {

int blockdevice_open(BlockDevice* dev)
{
    if (!dev || !dev->ops || !dev->ops->open)
        return -1;
    return dev->ops->open(dev);
}

int blockdevice_close(BlockDevice* dev)
{
    if (!dev || !dev->ops || !dev->ops->close)
        return -1;
    return dev->ops->close(dev);
}

LibC::ssize_t blockdevice_read(BlockDevice* dev, LibC::uint64_t block, void* buffer, LibC::size_t count)
{
    if (!dev || !dev->ops || !dev->ops->read)
        return -1;
    return dev->ops->read(dev, block, buffer, count);
}

LibC::ssize_t blockdevice_write(BlockDevice* dev, LibC::uint64_t block, void const* buffer, LibC::size_t count)
{
    if (!dev || !dev->ops || !dev->ops->write)
        return -1;
    return dev->ops->write(dev, block, buffer, count);
}

int blockdevice_ioctl(BlockDevice* dev, int request, void* arg)
{
    if (!dev || !dev->ops || !dev->ops->ioctl)
        return -1;
    return dev->ops->ioctl(dev, request, arg);
}

}
