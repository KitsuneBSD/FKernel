#include <LibFK/Memory/Allocators/heap_malloc.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Driver/Device/BlockDevice/block_device.h>

namespace fkernel {

fk::core::Result<size_t, fk::core::Error>
BlockDevice::read(uint64_t offset, size_t size, uint8_t* buffer) {
    const size_t ss = sector_size().value();
    if (ss == 0) return fk::core::Error::InvalidParameter;

    uint8_t* temp = static_cast<uint8_t*>(kmalloc(ss));
    if (!temp) return fk::core::Error::OutOfMemory;

    size_t bytes_read = 0;
    uint64_t current_offset = offset;

    while (bytes_read < size) {
        uint64_t lba = current_offset / ss;
        uint64_t sector_offset = current_offset % ss;
        size_t to_copy = (size - bytes_read < ss - sector_offset) ? (size - bytes_read) : (ss - sector_offset);

        auto read_res = read_sectors(lba, 1, temp);
        if (read_res.is_error()) {
            kfree(temp);
            return read_res.error();
        }

        fk::memory::copy(buffer + bytes_read, temp + sector_offset, to_copy);
        bytes_read += to_copy;
        current_offset += to_copy;
    }

    kfree(temp);
    return bytes_read;
}

fk::core::Result<size_t, fk::core::Error>
BlockDevice::write(uint64_t offset, size_t size, const uint8_t* buffer) {
    const size_t ss = sector_size().value();
    if (ss == 0) return fk::core::Error::InvalidParameter;

    uint8_t* temp = static_cast<uint8_t*>(kmalloc(ss));
    if (!temp) return fk::core::Error::OutOfMemory;

    size_t bytes_written = 0;
    uint64_t current_offset = offset;

    while (bytes_written < size) {
        uint64_t lba = current_offset / ss;
        uint64_t sector_offset = current_offset % ss;
        size_t to_copy = (size - bytes_written < ss - sector_offset) ? (size - bytes_written) : (ss - sector_offset);

        // If writing a partial sector, we must read it first
        if (to_copy < ss) {
            auto read_res = read_sectors(lba, 1, temp);
            if (read_res.is_error()) {
                kfree(temp);
                return read_res.error();
            }
        }

        fk::memory::copy(temp + sector_offset, buffer + bytes_written, to_copy);
        
        auto write_res = write_sectors(lba, 1, temp);
        if (write_res.is_error()) {
            kfree(temp);
            return write_res.error();
        }

        bytes_written += to_copy;
        current_offset += to_copy;
    }

    kfree(temp);
    return bytes_written;
}

size_t BlockDevice::size() const {
    return sector_count().value() * sector_size().value();
}

} // namespace fkernel
