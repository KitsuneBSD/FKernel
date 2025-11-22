#include <Kernel/Driver/Ata/AtaBlockCache.h>
#include <Kernel/Driver/Ata/AtaBlockDevice.h>
#include <Kernel/Driver/Ata/AtaController.h>
#include <LibFK/Algorithms/log.h>

AtaBlockDevice::AtaBlockDevice(fkernel::drivers::ata::AtaDeviceIoInfo device_io_info,
                               fk::memory::OwnPtr<fkernel::drivers::ata::AtaIoStrategy> io_strategy)
    : m_device_io_info(fk::types::move(device_io_info))
    , m_io_strategy(fk::types::move(io_strategy)) {}

int AtaBlockDevice::open(VNode *vnode, FileDescriptor *fd, int flags) {
  (void)vnode;
  (void)fd;
  (void)flags;
  return 0;
}

int AtaBlockDevice::close(VNode *vnode, FileDescriptor *fd) {
  (void)vnode;
  (void)fd;
  return 0;
}

int AtaBlockDevice::read_sectors(uint32_t lba, uint8_t sector_count, void* buffer) const {
  if (!m_io_strategy)
    return -1;
  return m_io_strategy->read_sectors(m_device_io_info, lba, sector_count, buffer);
}

int AtaBlockDevice::write_sectors(uint32_t lba, uint8_t sector_count, const void* buffer) {
  if (!m_io_strategy)
    return -1;
  return m_io_strategy->write_sectors(m_device_io_info, lba, sector_count, buffer);
}

int AtaBlockDevice::read(VNode *vnode, FileDescriptor *fd, void *buffer,
                         size_t size, size_t offset) const {
  (void)vnode; // Silence unused parameter warning
  (void)fd;

  char *out = reinterpret_cast<char *>(buffer);
  size_t remaining = size;
  uint32_t sector = offset / 512;
  size_t sector_offset = offset % 512;

  while (remaining > 0) {
    uint8_t *data = AtaCache::the().get_sector(this, sector);
    size_t to_copy = (remaining < 512 - sector_offset) ? remaining : (512 - sector_offset);
    memcpy(out, data + sector_offset, to_copy);
    remaining -= to_copy;
    out += to_copy;
    sector_offset = 0;
    sector++;
  }

  return static_cast<int>(size);
}

int AtaBlockDevice::write(VNode *vnode, FileDescriptor *fd, const void *buffer,
                          size_t size, size_t offset) {
  (void)vnode; // Silence unused parameter warning
  (void)fd;

  const char *in = reinterpret_cast<const char *>(buffer);
  size_t remaining = size;
  uint32_t sector = offset / 512;
  size_t sector_offset = offset % 512;

  AtaCache::the().flush(this); // Ensure all pending writes in cache are committed

  while (remaining > 0) {
    uint8_t *data = AtaCache::the().get_sector(this, sector);
    size_t to_copy = (remaining < 512 - sector_offset) ? remaining : (512 - sector_offset);
    memcpy(data + sector_offset, in, to_copy);
    AtaCache::the().mark_dirty(sector);
    remaining -= to_copy;
    in += to_copy;
    sector_offset = 0;
    sector++;
  }

  return static_cast<int>(size);
}
