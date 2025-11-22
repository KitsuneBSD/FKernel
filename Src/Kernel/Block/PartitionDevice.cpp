#include <Kernel/Block/PartitionDevice.h>
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/Posix/errno.h>
#include <LibFK/Algorithms/log.h>

PartitionBlockDevice::PartitionBlockDevice(fk::memory::RetainPtr<BlockDevice> device, PartitionLocation&& location)
    : m_device(fk::types::move(device))
    , m_location(fk::types::move(location)) {}

int PartitionBlockDevice::open(VNode *vnode, FileDescriptor *file_descriptor, int flags) {
  (void)vnode;
  (void)file_descriptor;
  (void)flags;
  return 0;
}

int PartitionBlockDevice::close(VNode *vnode, FileDescriptor *file_descriptor) {
  (void)vnode;
  (void)file_descriptor;
  return 0;
}

size_t PartitionBlockDevice::adjust_and_get_final_offset(size_t& size, size_t offset) const {
    size_t partition_size_bytes = m_location.size_in_bytes();

    if (offset >= partition_size_bytes) {
        size = 0;
        return 0;
    }

    if (offset + size > partition_size_bytes) {
        size = partition_size_bytes - offset;
    }

    return m_location.absolute_offset(offset);
}

int PartitionBlockDevice::read(VNode *vnode, FileDescriptor *file_descriptor, void *buffer,
                               size_t size, size_t offset) const {
  if (!m_device) {
    errno = EFAULT;
    return -1;
  }

  size_t final_offset = adjust_and_get_final_offset(size, offset);
  if (size == 0)
      return 0;

  return m_device->read(vnode, file_descriptor, buffer, size, final_offset);
}

int PartitionBlockDevice::write(VNode *vnode, FileDescriptor *file_descriptor,
                                const void *buffer, size_t size,
                                size_t offset) {
  if (!m_device) {
    errno = EFAULT;
    return -1;
  }

  size_t final_offset = adjust_and_get_final_offset(size, offset);
  if (size == 0)
      return -1; // Cannot write 0 bytes, may indicate writing past the end

  return m_device->write(vnode, file_descriptor, buffer, size, final_offset);
}

int PartitionBlockDevice::read_sectors(uint32_t lba, uint8_t sector_count, void* buffer) const {
    if (!m_device)
        return -1;
    // Adjust LBA by partition's starting LBA
    return m_device->read_sectors(m_location.first_lba() + lba, sector_count, buffer);
}

int PartitionBlockDevice::write_sectors(uint32_t lba, uint8_t sector_count, const void* buffer) {
    if (!m_device)
        return -1;
    // Adjust LBA by partition's starting LBA
    return m_device->write_sectors(m_location.first_lba() + lba, sector_count, buffer);
}
