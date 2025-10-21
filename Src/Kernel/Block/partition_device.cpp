#include <Kernel/Block/partition_device.h>
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/Driver/Ata/AtaBlockCache.h>
#include <LibFK/Algorithms/log.h>

int PartitionBlockDevice::open(VNode *vnode, FileDescriptor *fd, int flags)
{
    (void)fd; (void)flags;
    if (!vnode || !vnode->fs_private)
        return -1;
    return 0;
}

int PartitionBlockDevice::close(VNode *vnode, FileDescriptor *fd)
{
    (void)vnode; (void)fd;
    return 0;
}

int PartitionBlockDevice::read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size, size_t offset)
{
    (void)fd;
    if (!vnode || !vnode->fs_private)
        return -1;

    auto *pinfo = reinterpret_cast<PartitionInfo *>(vnode->fs_private);
    if (!pinfo || !pinfo->device)
        return -1;

    // Translate offset to LBA
    if (offset + size > (size_t)pinfo->sectors_count * 512)
        return -1; // out of range

    uint32_t start_sector = pinfo->lba_first + (offset / 512);
    size_t sector_offset = offset % 512;
    size_t remaining = size;
    char *out = reinterpret_cast<char *>(buffer);

    while (remaining > 0)
    {
        uint8_t *data = AtaCache::the().get_sector(pinfo->device, start_sector);
        size_t to_copy = (remaining < 512 - sector_offset) ? remaining : (512 - sector_offset);
        memcpy(out, data + sector_offset, to_copy);
        remaining -= to_copy;
        out += to_copy;
        sector_offset = 0;
        ++start_sector;
    }

    return static_cast<int>(size);
}

int PartitionBlockDevice::write(VNode *vnode, FileDescriptor *fd, const void *buffer, size_t size, size_t offset)
{
    (void)fd;
    if (!vnode || !vnode->fs_private)
        return -1;

    auto *pinfo = reinterpret_cast<PartitionInfo *>(vnode->fs_private);
    if (!pinfo || !pinfo->device)
        return -1;

    if (offset + size > (size_t)pinfo->sectors_count * 512)
        return -1; // out of range

    uint32_t start_sector = pinfo->lba_first + (offset / 512);
    size_t sector_offset = offset % 512;
    size_t remaining = size;
    const char *in = reinterpret_cast<const char *>(buffer);

    while (remaining > 0)
    {
        uint8_t *data = AtaCache::the().get_sector(pinfo->device, start_sector);
        size_t to_copy = (remaining < 512 - sector_offset) ? remaining : (512 - sector_offset);
        memcpy(data + sector_offset, in, to_copy);
        AtaCache::the().mark_dirty(start_sector);
        remaining -= to_copy;
        in += to_copy;
        sector_offset = 0;
        ++start_sector;
    }

    return static_cast<int>(size);
}

VNodeOps PartitionBlockDevice::ops = {
    .read = &PartitionBlockDevice::read,
    .write = &PartitionBlockDevice::write,
    .open = &PartitionBlockDevice::open,
    .close = &PartitionBlockDevice::close,
    .lookup = nullptr,
    .create = nullptr,
    .readdir = nullptr,
    .unlink = nullptr
};
