#include <Kernel/Driver/Ata/AtaBlockDevice.h>
#include <Kernel/Driver/Ata/AtaController.h>
#include <LibFK/Algorithms/log.h>

int AtaBlockDevice::open(VNode *vnode, int flags)
{
    (void)vnode;
    (void)flags;
    return 0;
}

int AtaBlockDevice::close(VNode *vnode)
{
    (void)vnode;
    return 0;
}

int AtaBlockDevice::read(VNode *vnode, void *buffer, size_t size, size_t offset)
{
    if (!vnode->fs_private)
        return -1;

    auto *info = reinterpret_cast<AtaDeviceInfo *>(vnode->fs_private);
    if (!info->exists)
        return -1;

    if (offset % 512 != 0)
    {
        // não suporta offset desalinhado ainda
        return -1;
    }

    uint32_t sector = offset / 512;
    uint8_t count = (size + 511) / 512;

    int bytes = AtaController::the().read_sectors_pio(*info, sector, count, buffer);
    return bytes;
}

int AtaBlockDevice::write(VNode *vnode, const void *buffer, size_t size, size_t offset)
{
    (void)vnode;
    (void)buffer;
    (void)size;
    (void)offset;
    // escrever: não implementado ainda
    return -1;
}

VNodeOps AtaBlockDevice::ops = {
    .read = &AtaBlockDevice::read,
    .write = &AtaBlockDevice::write,
    .open = &AtaBlockDevice::open,
    .close = &AtaBlockDevice::close,
    .lookup = nullptr,
    .create = nullptr,
    .readdir = nullptr,
    .unlink = nullptr};