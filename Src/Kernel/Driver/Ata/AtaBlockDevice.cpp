#include <Kernel/Driver/Ata/AtaBlockCache.h>
#include <Kernel/Driver/Ata/AtaBlockDevice.h>
#include <Kernel/Driver/Ata/AtaController.h>
#include <LibFK/Algorithms/log.h>

AtaBlockDevice::AtaBlockDevice(
    fkernel::drivers::ata::AtaDeviceIoInfo device_io_info,
    fk::memory::OwnPtr<fkernel::drivers::ata::AtaIoStrategy> io_strategy)
    : m_device_io_info(fk::types::move(device_io_info)),
      m_io_strategy(fk::types::move(io_strategy)) {
  fk::algorithms::klog("ATA BLOCK DEVICE",
                       "AtaBlockDevice created. Bus: %d, Drive: %d",
                       static_cast<int>(m_device_io_info.bus()),
                       static_cast<int>(m_device_io_info.drive()));
}

int AtaBlockDevice::open(VNode *vnode, FileDescriptor *fd, int flags) {
  (void)vnode;
  (void)fd;
  (void)flags;
  fk::algorithms::klog("ATA BLOCK DEVICE",
                       "Open called for ATA device. Bus: %d, Drive: %d",
                       static_cast<int>(m_device_io_info.bus()),
                       static_cast<int>(m_device_io_info.drive()));
  return 0;
}

int AtaBlockDevice::close(VNode *vnode, FileDescriptor *fd) {
  (void)vnode;
  (void)fd;
  fk::algorithms::klog("ATA BLOCK DEVICE",
                       "Close called for ATA device. Bus: %d, Drive: %d",
                       static_cast<int>(m_device_io_info.bus()),
                       static_cast<int>(m_device_io_info.drive()));
  return 0;
}

int AtaBlockDevice::read_sectors(uint32_t lba, uint8_t sector_count,
                                 void *buffer) const {
  fk::algorithms::klog(
      "ATA BLOCK DEVICE",
      "Read sectors called. LBA: %u, Count: %u. Bus: %d, Drive: %d", lba,
      sector_count, static_cast<int>(m_device_io_info.bus()),
      static_cast<int>(m_device_io_info.drive()));
  if (!m_io_strategy) {
    fk::algorithms::kerror("ATA BLOCK DEVICE",
                           "Read sectors failed: I/O strategy is null.");
    return -1;
  }
  int sectors_read =
      m_io_strategy->read_sectors(m_device_io_info, lba, sector_count, buffer);
  if (sectors_read < 0) {
    fk::algorithms::kerror(
        "ATA BLOCK DEVICE",
        "Read sectors failed from I/O strategy for LBA %u, count %u. Error: %d",
        lba, sector_count, sectors_read);
  } else {
    fk::algorithms::klog("ATA BLOCK DEVICE",
                         "Read %d sectors (LBA %u, count %u) via I/O strategy.",
                         sectors_read, lba, sector_count);
  }
  return sectors_read;
}

int AtaBlockDevice::write_sectors(uint32_t lba, uint8_t sector_count,
                                  const void *buffer) {
  fk::algorithms::klog(
      "ATA BLOCK DEVICE",
      "Write sectors called. LBA: %u, Count: %u. Bus: %d, Drive: %d", lba,
      sector_count, static_cast<int>(m_device_io_info.bus()),
      static_cast<int>(m_device_io_info.drive()));
  if (!m_io_strategy) {
    fk::algorithms::kerror("ATA BLOCK DEVICE",
                           "Write sectors failed: I/O strategy is null.");
    return -1;
  }
  int sectors_written =
      m_io_strategy->write_sectors(m_device_io_info, lba, sector_count, buffer);
  if (sectors_written < 0) {
    fk::algorithms::kerror("ATA BLOCK DEVICE",
                           "Write sectors failed from I/O strategy for LBA %u, "
                           "count %u. Error: %d",
                           lba, sector_count, sectors_written);
  } else {
    fk::algorithms::klog(
        "ATA BLOCK DEVICE",
        "Written %d sectors (LBA %u, count %u) via I/O strategy.",
        sectors_written, lba, sector_count);
  }
  return sectors_written;
}

int AtaBlockDevice::read(VNode *vnode, FileDescriptor *fd, void *buffer,
                         size_t size, size_t offset) const {
  fk::algorithms::klog(
      "ATA BLOCK DEVICE",
      "Read data called. Size: %zu, Offset: %zu. Bus: %d, Drive: %d", size,
      offset, static_cast<int>(m_device_io_info.bus()),
      static_cast<int>(m_device_io_info.drive()));

  (void)vnode; // Silence unused parameter warning
  (void)fd;

  if (!buffer) {
    fk::algorithms::kerror("ATA BLOCK DEVICE", "Read failed: buffer is null.");
    return -1; // Invalid buffer
  }
  if (size == 0) {
    fk::algorithms::klog("ATA BLOCK DEVICE",
                         "Read called with size 0. Returning 0.");
    return 0;
  }

  char *out = reinterpret_cast<char *>(buffer);
  size_t remaining = size;
  uint32_t sector = offset / 512;
  size_t sector_offset = offset % 512;

  while (remaining > 0) {
    fk::algorithms::kdebug(
        "ATA BLOCK DEVICE",
        "Reading block device LBA %u (offset %zu), remaining %zu bytes.",
        sector, sector_offset, remaining);
    uint8_t *data = AtaCache::the().get_sector(this, sector);
    if (!data) {
      fk::algorithms::kerror(
          "ATA BLOCK DEVICE",
          "Failed to get sector %u from cache/device during read.", sector);
      return static_cast<int>(
          size - remaining); // Return bytes successfully read so far
    }
    size_t to_copy =
        (remaining < 512 - sector_offset) ? remaining : (512 - sector_offset);
    memcpy(out, data + sector_offset, to_copy);
    remaining -= to_copy;
    out += to_copy;
    sector_offset = 0;
    sector++;
  }

  fk::algorithms::klog("ATA BLOCK DEVICE",
                       "Finished reading %zu bytes from ATA device.", size);
  return static_cast<int>(size);
}

int AtaBlockDevice::write(VNode *vnode, FileDescriptor *fd, const void *buffer,
                          size_t size, size_t offset) {
  fk::algorithms::klog(
      "ATA BLOCK DEVICE",
      "Write data called. Size: %zu, Offset: %zu. Bus: %d, Drive: %d", size,
      offset, static_cast<int>(m_device_io_info.bus()),
      static_cast<int>(m_device_io_info.drive()));

  (void)vnode; // Silence unused parameter warning
  (void)fd;

  if (!buffer) {
    fk::algorithms::kerror("ATA BLOCK DEVICE", "Write failed: buffer is null.");
    return -1; // Invalid buffer
  }
  if (size == 0) {
    fk::algorithms::klog("ATA BLOCK DEVICE",
                         "Write called with size 0. Nothing to write.");
    return 0; // Nothing to write
  }

  const char *in = reinterpret_cast<const char *>(buffer);
  size_t remaining = size;
  uint32_t sector = offset / 512;
  size_t sector_offset = offset % 512;

  AtaCache::the().flush(
      this); // Ensure all pending writes in cache are committed
  fk::algorithms::klog("ATA BLOCK DEVICE",
                       "Flushed ATA cache before write operation.");

  while (remaining > 0) {
    fk::algorithms::kdebug(
        "ATA BLOCK DEVICE",
        "Writing block device LBA %u (offset %zu), remaining %zu bytes.",
        sector, sector_offset, remaining);
    uint8_t *data = AtaCache::the().get_sector(this, sector);
    if (!data) {
      fk::algorithms::kerror(
          "ATA BLOCK DEVICE",
          "Failed to get sector %u from cache/device during write.", sector);
      return static_cast<int>(
          size - remaining); // Return bytes successfully written so far
    }
    size_t to_copy =
        (remaining < 512 - sector_offset) ? remaining : (512 - sector_offset);
    memcpy(data + sector_offset, in, to_copy);
    AtaCache::the().mark_dirty(sector);
    remaining -= to_copy;
    in += to_copy;
    sector_offset = 0;
    sector++;
  }

  fk::algorithms::klog("ATA BLOCK DEVICE",
                       "Finished writing %zu bytes to ATA device.", size);
  AtaCache::the().flush(
      this); // Flush again after all writes are done to ensure persistence
  fk::algorithms::klog("ATA BLOCK DEVICE",
                       "Flushed ATA cache after write operation.");

  return static_cast<int>(size);
}
