#include <Kernel/Driver/Devices/zero_device.h>
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <LibC/stddef.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

int dev_zero_read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size,
                  size_t offset) {
  (void)vnode;
  (void)fd;
  (void)offset;
  if (buffer && size > 0)
    memset(buffer, 0, size);
  return static_cast<int>(size);
}

int dev_zero_write(VNode *vnode, FileDescriptor *fd, const void *buffer,
                   size_t size, size_t offset) {
  (void)vnode;
  (void)fd;
  (void)buffer;
  (void)offset;
  return static_cast<int>(size);
}

VNodeOps zero_ops = {.read = &dev_zero_read,
                     .write = &dev_zero_write,
                     .open = nullptr,
                     .close = nullptr,
                     .lookup = nullptr,
                     .create = nullptr,
                     .readdir = nullptr,
                     .unlink = nullptr};
