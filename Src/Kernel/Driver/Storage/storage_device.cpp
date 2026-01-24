#include <Kernel/Driver/Storage/storage_device.h>
#include <LibFK/Memory/heap_malloc.h>
#include <LibFK/Utilities/size_checking.h>
#include <LibC/string.h>

fk::core::Result<size_t, fk::core::Error>
StorageDevice::read(uint64_t offset, size_t size, uint8_t *buffer) {
  const size_t ss = sector_size().value();
  uint8_t* temp = static_cast<uint8_t*>(kmalloc(ss));
  if (!temp) return fk::core::Error::OutOfMemory;

  size_t bytes_read = 0;
  uint64_t current_offset = offset;

  while (bytes_read < size) {
    uint64_t lba = current_offset / ss;
    uint64_t sector_offset = current_offset % ss;
    size_t to_copy = fk::utilities::min(size - bytes_read, ss - sector_offset);

    if (read_sectors(lba, 1, temp).is_error()) {
        kfree(temp);
        return fk::core::Error::IOError;
    }

    memcpy(buffer + bytes_read, temp + sector_offset, to_copy);
    bytes_read += to_copy;
    current_offset += to_copy;
  }

  kfree(temp);
  return bytes_read;
}

fk::core::Result<size_t, fk::core::Error>
StorageDevice::write(uint64_t offset, size_t size, const uint8_t *buffer) {
  const size_t ss = sector_size().value();
  uint8_t* temp = static_cast<uint8_t*>(kmalloc(ss));
  if (!temp) return fk::core::Error::OutOfMemory;

  size_t bytes_written = 0;
  uint64_t current_offset = offset;

  while (bytes_written < size) {
    uint64_t lba = current_offset / ss;
    uint64_t sector_offset = current_offset % ss;
    size_t to_copy = fk::utilities::min(size - bytes_written, ss - sector_offset);

    // If writing a partial sector, we must read it first
    if (to_copy < ss) {
        read_sectors(lba, 1, temp);
    }

    memcpy(temp + sector_offset, buffer + bytes_written, to_copy);
    
    if (write_sectors(lba, 1, temp).is_error()) {
        kfree(temp);
        return fk::core::Error::IOError;
    }

    bytes_written += to_copy;
    current_offset += to_copy;
  }

  kfree(temp);
  return bytes_written;
}

size_t StorageDevice::size() const {
  return sector_count().value() * sector_size().value();
}
