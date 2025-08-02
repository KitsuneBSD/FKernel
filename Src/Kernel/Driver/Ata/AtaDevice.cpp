#include <Kernel/Devices/Storage/BlockDevice.h>
#include <Kernel/Devices/Storage/BlockDeviceTypes.h>
#include <Kernel/Driver/Ata/AtaController.h>
#include <Kernel/Driver/Ata/AtaDevice.h>
#include <Kernel/Driver/Ata/AtaTypes.h>
#include <Kernel/FileSystem/VFS/VFSTypes.h>
#include <Kernel/Posix/errno.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/enforce.h>
#include <LibFK/types.h>

int ata_open(Device::BlockDevice* dev)
{
    if (!dev)
        return -EINVAL;

    auto* node = Device::blockdevice_node_from_device(dev);
    (void)node;
    return 0;
}

int ata_close(Device::BlockDevice* dev)
{
    if (!dev)
        return -EINVAL;

    auto* node = Device::blockdevice_node_from_device(dev);
    (void)node;
    return 0;
}

LibC::ssize_t ata_read(Device::BlockDevice* dev, LibC::uint64_t block, void* buffer, LibC::size_t count)
{
    if (!dev || !buffer || count == 0)
        return -EINVAL;

    auto* node = Device::blockdevice_node_from_device(dev);

    LibC::uint32_t lba = static_cast<LibC::uint32_t>(block);
    LibC::uint8_t sector_count = static_cast<LibC::uint8_t>(count);

    bool ok = ATAController::instance().read_sectors(
        node->channel, node->drive, lba, sector_count, buffer);

    return ok ? static_cast<LibC::ssize_t>(sector_count) : -EIO;
}

LibC::ssize_t ata_write(Device::BlockDevice* dev, LibC::uint64_t block, void const* buffer, LibC::size_t count)
{
    if (!dev || !buffer || count == 0)
        return -EINVAL;

    auto* node = Device::blockdevice_node_from_device(dev);

    LibC::uint32_t lba = static_cast<LibC::uint32_t>(block);
    LibC::uint8_t sector_count = static_cast<LibC::uint8_t>(count);

    bool ok = ATAController::instance().write_sectors(
        node->channel, node->drive, lba, sector_count, buffer);

    return ok ? static_cast<LibC::ssize_t>(sector_count) : -EIO;
}

int ata_ioctl(Device::BlockDevice* dev, int request, void* arg)
{
    (void)dev;
    (void)request;
    (void)arg;

    return -ENOTTY;
}

Device::BlockDeviceOps AtaBlockDeviceOps = {
    .open = ata_open,
    .close = ata_close,
    .read = ata_read,
    .write = ata_write,
    .ioctl = ata_ioctl,
};
