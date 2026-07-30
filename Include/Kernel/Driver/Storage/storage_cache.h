#pragma once

#include <Kernel/Driver/Storage/storage_device.h>
#include <LibFK/Container/vector.h>

class StorageCache : public StorageDevice {
    fk::RefPtr<StorageDevice> m_device;
    
    struct CacheEntry {
        uint64_t sector;
        uint8_t data[512];
        bool valid { false };
        bool dirty { false };
    };

    static constexpr size_t CACHE_SIZE = 64;
    CacheEntry m_cache[CACHE_SIZE];

public:
    explicit StorageCache(fk::RefPtr<StorageDevice> device)
        : m_device(fk::types::move(device)) {}

    virtual fk::core::Result<size_t, fk::core::Error>
    read_sectors(uint64_t start_sector, size_t count, uint8_t* buffer) override;

    virtual fk::core::Result<size_t, fk::core::Error>
    write_sectors(uint64_t start_sector, size_t count, const uint8_t* buffer) override;

    virtual SectorSize sector_size() const override { return m_device->sector_size(); }
    virtual SectorCount sector_count() const override { return m_device->sector_count(); }

private:
    CacheEntry* find_entry(uint64_t sector);
};
