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

static int block_device_read(VNode *vnode, FileDescriptor *file_descriptor, void *buffer,
                             size_t size, size_t offset) {
  BlockDevice *device = get_block_device(vnode);
  if (!device)
    return -1;
  return device->read(vnode, file_descriptor, buffer, size, offset);
}

static int block_device_write(VNode *vnode, FileDescriptor *file_descriptor,
                              const void *buffer, size_t size, size_t offset) {
  BlockDevice *device = get_block_device(vnode);
  if (!device)
    return -1;
  return device->write(vnode, file_descriptor, buffer, size, offset);
}

static int block_device_open(VNode *vnode, FileDescriptor *file_descriptor, int flags) {
  BlockDevice *device = get_block_device(vnode);
  if (!device)
    return -1;
  return device->open(vnode, file_descriptor, flags);
}

static int block_device_close(VNode *vnode, FileDescriptor *file_descriptor) {
  BlockDevice *device = get_block_device(vnode);
  if (!device)
    return -1;
  return device->close(vnode, file_descriptor);
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
