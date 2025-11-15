#include <Kernel/Block/PartitionDevice.h>
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/Posix/errno.h>
#include <LibFK/Algorithms/log.h>

PartitionBlockDevice::PartitionBlockDevice(PartitionInfo &&info)
    : m_info(move(info)) {}

int PartitionBlockDevice::open(VNode *vnode, FileDescriptor *fd, int flags) {
  (void)vnode;
  (void)fd;
  (void)flags;
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
  if (!m_info.device) {
    errno = EFAULT;
    return -1;
  }

  if (offset >= (size_t)m_info.sectors_count * 512) {
    return 0;
  }

  if (offset + size > (size_t)m_info.sectors_count * 512) {
    size = (size_t)m_info.sectors_count * 512 - offset;
  }

  return m_info.device->read(vnode, fd, buffer, size,
                             (size_t)m_info.lba_first * 512 + offset);
}

int PartitionBlockDevice::write(VNode *vnode, FileDescriptor *fd,
                                const void *buffer, size_t size,
                                size_t offset) {
  (void)fd;
  if (!m_info.device) {
    errno = EFAULT;
    return -1;
  }

  if (offset >= (size_t)m_info.sectors_count * 512) {
    return -1;
  }

  if (offset + size > (size_t)m_info.sectors_count * 512) {
    size = (size_t)m_info.sectors_count * 512 - offset;
  }

  return m_info.device->write(vnode, fd, buffer, size,
                              (size_t)m_info.lba_first * 512 + offset);
}
