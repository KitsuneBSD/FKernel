#include <Kernel/Driver/Devices/zero_device.h>
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <LibC/stddef.h>
#include <LibFK/Algorithms/log.h>
#include <LibC/string.h>

int dev_zero_read(VNode *vnode, void *buffer, size_t size, size_t offset)
{
    (void)vnode;
    (void)offset;
    if (buffer && size > 0)
        memset(buffer, 0, size);
    return static_cast<int>(size);
}

int dev_zero_write(VNode *vnode, const void *buffer, size_t size, size_t offset)
{
    (void)vnode;
    (void)buffer;
    (void)offset;
    return static_cast<int>(size);
}

// -------------------
// VNodeOps para /dev/zero
// -------------------
VNodeOps zero_ops = {
    .read = &dev_zero_read,
    .write = &dev_zero_write,
    .open = nullptr,
    .close = nullptr,
    .lookup = nullptr,
    .create = nullptr,
    .readdir = nullptr,
    .unlink = nullptr};
