#include <Kernel/Block/PartitionDevice.h>
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/Posix/errno.h>
#include <LibFK/Algorithms/log.h>

PartitionBlockDevice::PartitionBlockDevice(
    fk::memory::RetainPtr<BlockDevice> device, PartitionLocation &&location)
    : m_device(fk::types::move(device)), m_location(fk::types::move(location)) {
  fk::algorithms::klog(
      "PARTITION DEVICE",
      "PartitionBlockDevice created. LBA Start: %u, Sector Count: %u",
      m_location.first_lba(), m_location.sector_count());
}

int PartitionBlockDevice::open(VNode *vnode, FileDescriptor *file_descriptor,
                               int flags) {
  (void)vnode;
  (void)file_descriptor;
  (void)flags;
  fk::algorithms::klog("PARTITION DEVICE",
                       "Open called for partition device. LBA Start: %u",
                       m_location.first_lba());
  return 0;
}

int PartitionBlockDevice::close(VNode *vnode, FileDescriptor *file_descriptor) {
  (void)vnode;
  (void)file_descriptor;
  fk::algorithms::klog("PARTITION DEVICE",
                       "Close called for partition device. LBA Start: %u",
                       m_location.first_lba());
  return 0;
}

size_t PartitionBlockDevice::adjust_and_get_final_offset(size_t &size,
                                                         size_t offset) const {
  size_t partition_size_bytes = m_location.size_in_bytes();

  fk::algorithms::klog(
      "PARTITION DEVICE",
      "Adjusting offset for read/write. Original offset: %zu, size: %zu. "
      "Partition LBA Start: %u, Partition Size: %zu bytes.",
      offset, size, m_location.first_lba(), partition_size_bytes);

  if (offset >= partition_size_bytes) {
    fk::algorithms::kwarn("PARTITION DEVICE",
                          "Offset %zu is beyond partition boundary %zu. No "
                          "data will be read/written.",
                          offset, partition_size_bytes);
    size = 0;
    return 0;
  }

  if (offset + size > partition_size_bytes) {
    size_t old_size = size;
    size = partition_size_bytes - offset;
    fk::algorithms::kwarn("PARTITION DEVICE",
                          "Read/write size %zu exceeds partition boundary. "
                          "Truncating to %zu bytes.",
                          old_size, size);
  }

  size_t final_offset = m_location.absolute_offset(offset);
  fk::algorithms::klog(
      "PARTITION DEVICE",
      "Adjusted for partition. Final absolute offset: %zu, Adjusted size: %zu.",
      final_offset, size);
  return final_offset;
}

int PartitionBlockDevice::read(VNode *vnode, FileDescriptor *file_descriptor,
                               void *buffer, size_t size, size_t offset) const {
  fk::algorithms::klog("PARTITION DEVICE",
                       "Read called. Size: %zu, Offset: %zu, LBA Start: %u.",
                       size, offset, m_location.first_lba());
  if (!m_device) {
    fk::algorithms::kerror("PARTITION DEVICE",
                           "Read failed: Underlying device is null.");
    errno = EFAULT;
    return -1;
  }

  size_t final_offset = adjust_and_get_final_offset(size, offset);
  if (size == 0)
    return 0;

  int bytes_read =
      m_device->read(vnode, file_descriptor, buffer, size, final_offset);
  if (bytes_read < 0) {
    fk::algorithms::kerror(
        "PARTITION DEVICE",
        "Underlying device read failed for partition (LBA %u). Error: %d",
        m_location.first_lba(), bytes_read);
  } else {
    fk::algorithms::klog(
        "PARTITION DEVICE",
        "Read %d bytes from partition (LBA %u, absolute offset %zu).",
        bytes_read, m_location.first_lba(), final_offset);
  }
  return bytes_read;
}

int PartitionBlockDevice::write(VNode *vnode, FileDescriptor *file_descriptor,
                                const void *buffer, size_t size,
                                size_t offset) {
  fk::algorithms::klog("PARTITION DEVICE",
                       "Write called. Size: %zu, Offset: %zu, LBA Start: %u.",
                       size, offset, m_location.first_lba());
  if (!m_device) {
    fk::algorithms::kerror("PARTITION DEVICE",
                           "Write failed: Underlying device is null.");
    errno = EFAULT;
    return -1;
  }

  size_t final_offset = adjust_and_get_final_offset(size, offset);
  if (size == 0) {
    fk::algorithms::kwarn("PARTITION DEVICE",
                          "Write called with adjusted size 0. Nothing written "
                          "for partition (LBA %u).");
    return -1; // Cannot write 0 bytes, may indicate writing past the end
  }

  int bytes_written =
      m_device->write(vnode, file_descriptor, buffer, size, final_offset);
  if (bytes_written < 0) {
    fk::algorithms::kerror(
        "PARTITION DEVICE",
        "Underlying device write failed for partition (LBA %u). Error: %d",
        m_location.first_lba(), bytes_written);
  } else {
    fk::algorithms::klog(
        "PARTITION DEVICE",
        "Written %d bytes to partition (LBA %u, absolute offset %zu).",
        bytes_written, m_location.first_lba(), final_offset);
  }
  return bytes_written;
}

int PartitionBlockDevice::read_sectors(uint32_t lba, uint8_t sector_count,
                                       void *buffer) const {
  fk::algorithms::klog(
      "PARTITION DEVICE",
      "Read sectors called. LBA: %u, Count: %u, Partition LBA Start: %u.", lba,
      sector_count, m_location.first_lba());
  if (!m_device) {
    fk::algorithms::kerror("PARTITION DEVICE",
                           "Read sectors failed: Underlying device is null.");
    return -1;
  }
  uint32_t absolute_lba = m_location.first_lba() + lba;
  int sectors_read = m_device->read_sectors(absolute_lba, sector_count, buffer);
  if (sectors_read < 0) {
    fk::algorithms::kerror("PARTITION DEVICE",
                           "Underlying device read_sectors failed for "
                           "partition. Absolute LBA: %u, Error: %d",
                           absolute_lba, sectors_read);
  } else {
    fk::algorithms::klog("PARTITION DEVICE",
                         "Read %d sectors from partition. Absolute LBA: %u.",
                         sectors_read, absolute_lba);
  }
  return sectors_read;
}

int PartitionBlockDevice::write_sectors(uint32_t lba, uint8_t sector_count,
                                        const void *buffer) {
  fk::algorithms::klog(
      "PARTITION DEVICE",
      "Write sectors called. LBA: %u, Count: %u, Partition LBA Start: %u.", lba,
      sector_count, m_location.first_lba());
  if (!m_device) {
    fk::algorithms::kerror("PARTITION DEVICE",
                           "Write sectors failed: Underlying device is null.");
    return -1;
  }
  uint32_t absolute_lba = m_location.first_lba() + lba;
  int sectors_written =
      m_device->write_sectors(absolute_lba, sector_count, buffer);
  if (sectors_written < 0) {
    fk::algorithms::kerror("PARTITION DEVICE",
                           "Underlying device write_sectors failed for "
                           "partition. Absolute LBA: %u, Error: %d",
                           absolute_lba, sectors_written);
  } else {
    fk::algorithms::klog("PARTITION DEVICE",
                         "Written %d sectors to partition. Absolute LBA: %u.",
                         sectors_written, absolute_lba);
  }
  return sectors_written;
}
