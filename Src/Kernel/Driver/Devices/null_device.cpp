#include <Kernel/Driver/Devices/null_device.h>
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <LibC/stddef.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

int dev_null_read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size,
                  size_t offset) {
  (void)vnode;
  (void)fd;
  (void)buffer;
  (void)size;
  (void)offset;
  return 0; // EOF
}

int dev_null_write(VNode *vnode, FileDescriptor *fd, const void *buffer,
                   size_t size, size_t offset) {
  (void)fd;
  (void)vnode;
  (void)buffer;
  (void)offset;
  return static_cast<int>(size); // aceita tudo
}

VNodeOps null_ops = {.read = &dev_null_read,
                     .write = &dev_null_write,
                     .open = nullptr,
                     .close = nullptr,
                     .lookup = nullptr,
                     .create = nullptr,
                     .readdir = nullptr,
                     .unlink = nullptr};
