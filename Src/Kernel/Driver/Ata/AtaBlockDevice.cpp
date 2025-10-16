#include <Kernel/Driver/Ata/AtaBlockDevice.h>
#include <Kernel/Driver/Ata/AtaBlockCache.h>
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

    char *out = reinterpret_cast<char *>(buffer);
    size_t remaining = size;
    uint32_t sector = offset / 512;
    size_t sector_offset = offset % 512;

    while (remaining > 0)
    {
        uint8_t *data = AtaCache::the().get_sector(info, sector);
        size_t to_copy = (remaining < 512 - sector_offset) ? remaining : (512 - sector_offset);
        memcpy(out, data + sector_offset, to_copy);
        remaining -= to_copy;
        out += to_copy;
        sector_offset = 0;
        sector++;
    }

    return static_cast<int>(size);
}

int AtaBlockDevice::write(VNode *vnode, const void *buffer, size_t size, size_t offset)
{
    if (!vnode->fs_private)
        return -1;
    auto *info = reinterpret_cast<AtaDeviceInfo *>(vnode->fs_private);
    if (!info->exists)
        return -1;

    const char *in = reinterpret_cast<const char *>(buffer);
    size_t remaining = size;
    uint32_t sector = offset / 512;
    size_t sector_offset = offset % 512;

    while (remaining > 0)
    {
        uint8_t *data = AtaCache::the().get_sector(info, sector);
        size_t to_copy = (remaining < 512 - sector_offset) ? remaining : (512 - sector_offset);
        memcpy(data + sector_offset, in, to_copy);
        AtaCache::the().mark_dirty(sector);
        remaining -= to_copy;
        in += to_copy;
        sector_offset = 0;
        sector++;
    }

    return static_cast<int>(size);
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