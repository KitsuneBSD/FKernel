#include <Kernel/Block/BlockDevice.h>
#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/Posix/errno.h>

// Helper to cast fs_private to BlockDevice*
static inline BlockDevice *get_block_device(VNode *vnode) {
  if (!vnode || !vnode->fs_private) {
    errno = EFAULT;
    return nullptr;
  }
  return reinterpret_cast<BlockDevice *>(vnode->fs_private);
}

static int block_device_read(VNode *vnode, FileDescriptor *fd, void *buffer,
                             size_t size, size_t offset) {
  BlockDevice *dev = get_block_device(vnode);
  if (!dev)
    return -1;
  return dev->read(vnode, fd, buffer, size, offset);
}

static int block_device_write(VNode *vnode, FileDescriptor *fd,
                              const void *buffer, size_t size, size_t offset) {
  BlockDevice *dev = get_block_device(vnode);
  if (!dev)
    return -1;
  return dev->write(vnode, fd, buffer, size, offset);
}

static int block_device_open(VNode *vnode, FileDescriptor *fd, int flags) {
  BlockDevice *dev = get_block_device(vnode);
  if (!dev)
    return -1;
  return dev->open(vnode, fd, flags);
}

static int block_device_close(VNode *vnode, FileDescriptor *fd) {
  BlockDevice *dev = get_block_device(vnode);
  if (!dev)
    return -1;
  return dev->close(vnode, fd);
}

// Initialize the global VNodeOps for block devices
VNodeOps g_block_device_ops = {
    .read = block_device_read,
    .write = block_device_write,
    .open = block_device_open,
    .close = block_device_close,
    .lookup = nullptr, // Block devices typically don't have children
    .create = nullptr,
    .readdir = nullptr,
    .unlink = nullptr,
};
