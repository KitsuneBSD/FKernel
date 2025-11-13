#include <Kernel/Block/PartitionDevice.h>
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/Posix/errno.h>
#include <LibFK/Algorithms/log.h>

int PartitionBlockDevice::open(VNode *vnode, FileDescriptor *fd, int flags) {
  (void)fd;
  (void)flags;
  if (!vnode || !vnode->fs_private)
    return -1;
  return 0;
}

int PartitionBlockDevice::close(VNode *vnode, FileDescriptor *fd) {
  (void)vnode;
  (void)fd;
  return 0;
}

int PartitionBlockDevice::read(VNode *vnode, FileDescriptor *fd, void *buffer,
                               size_t size, size_t offset) {
  (void)fd;
  if (!vnode || !vnode->fs_private)
    return -1;

  auto *pinfo = reinterpret_cast<PartitionInfo *>(vnode->fs_private);
  if (!pinfo || !pinfo->device) {
    errno = EFAULT;
    return -1;
  }

  // Translate offset to LBA
  if (offset + size > (size_t)pinfo->sectors_count * 512)
    return -1; // out of range

  // Read from the underlying block device, adjusting the offset for the
  // partition
  return pinfo->device->read(vnode, fd, buffer, size,
                             pinfo->lba_first * 512 + offset);
}

int PartitionBlockDevice::write(VNode *vnode, FileDescriptor *fd,
                                const void *buffer, size_t size,
                                size_t offset) {
  (void)fd;
  if (!vnode || !vnode->fs_private)
    return -1;

  auto *pinfo = reinterpret_cast<PartitionInfo *>(vnode->fs_private);
  if (!pinfo || !pinfo->device) {
    errno = EFAULT;
    return -1;
  }

  if (offset + size > (size_t)pinfo->sectors_count * 512)
    return -1; // out of range

  // Write to the underlying block device, adjusting the offset for the
  // partition
  return pinfo->device->write(vnode, fd, buffer, size,
                              pinfo->lba_first * 512 + offset);
}
