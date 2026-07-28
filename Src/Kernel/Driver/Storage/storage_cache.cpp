#include <Kernel/Driver/Storage/storage_cache.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/memory.h>

StorageCache::CacheEntry* StorageCache::find_entry(uint64_t sector) {
    size_t index = sector % CACHE_SIZE;
    if (m_cache[index].valid && m_cache[index].sector == sector) {
        return &m_cache[index];
    }
    return nullptr;
}

fk::core::Result<size_t, fk::core::Error>
StorageCache::read_sectors(uint64_t start_sector, size_t count, uint8_t* buffer) {
    fk::algorithms::klog("STORAGE_CACHE", "Reading %u sectors at %llu", count, start_sector);
    
    for (size_t i = 0; i < count; ++i) {
        uint64_t sector = start_sector + i;
        CacheEntry* entry = find_entry(sector);
        
        if (entry) {
            fk::algorithms::klog("STORAGE_CACHE", "Hit for sector %llu", sector);
            fk::memory::copy(buffer + (i * 512), entry->data, 512);
        } else {
            fk::algorithms::klog("STORAGE_CACHE", "Miss for sector %llu, loading...", sector);
            size_t index = sector % CACHE_SIZE;
            if (m_cache[index].dirty) {
                TRY(m_device->write_sectors(m_cache[index].sector, 1, m_cache[index].data));
            }
            
            TRY(m_device->read_sectors(sector, 1, m_cache[index].data));
            m_cache[index].sector = sector;
            m_cache[index].valid = true;
            m_cache[index].dirty = false;
            
            fk::memory::copy(buffer + (i * 512), m_cache[index].data, 512);
        }
    }
    
    return count;
}

fk::core::Result<size_t, fk::core::Error>
StorageCache::write_sectors(uint64_t start_sector, size_t count, const uint8_t* buffer) {
    fk::algorithms::klog("STORAGE_CACHE", "Writing %u sectors at %llu", count, start_sector);
    
    for (size_t i = 0; i < count; ++i) {
        uint64_t sector = start_sector + i;
        size_t index = sector % CACHE_SIZE;
        
        fk::memory::copy(m_cache[index].data, buffer + (i * 512), 512);
        m_cache[index].sector = sector;
        m_cache[index].valid = true;
        m_cache[index].dirty = true;
    }
    
    for (size_t i = 0; i < count; ++i) {
        uint64_t sector = start_sector + i;
        size_t index = sector % CACHE_SIZE;
        TRY(m_device->write_sectors(sector, 1, m_cache[index].data));
        m_cache[index].dirty = false;
    }
    
    return count;
}
