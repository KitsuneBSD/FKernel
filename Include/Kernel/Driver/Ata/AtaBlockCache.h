#pragma once

#include <Kernel/Driver/Ata/AtaDefs.h>
#include <LibFK/Types/types.h>

/**
 * @brief Represents a single cached sector in the ATA cache
 */
struct SectorCacheEntry {
    uint32_t lba;      ///< LBA (Logical Block Address) of the cached sector
    bool is_valid;     ///< Whether this cache entry contains valid data
    bool is_dirty;     ///< Whether this sector has been modified and needs flushing
    uint8_t data[512]; ///< Sector data (typically 512 bytes)
};

/**
 * @brief Simple sector cache for ATA devices
 * 
 * Provides caching for up to 8 sectors to reduce physical ATA reads/writes.
 * Implements a singleton pattern for global access.
 */
class AtaCache {
private:
    AtaCache() = default;          ///< Private constructor for singleton
    SectorCacheEntry cache[8];     ///< Fixed-size sector cache

public:
    /**
     * @brief Get the singleton instance of AtaCache
     * @return Reference to the global AtaCache
     */
    static AtaCache &the();

    /**
     * @brief Retrieve a sector from the cache or read it from the device
     * 
     * @param device Pointer to the ATA device
     * @param lba Logical Block Address of the sector
     * @return Pointer to the cached sector data
     */
    uint8_t *get_sector(AtaDeviceInfo *device, uint32_t lba);

    /**
     * @brief Mark a cached sector as dirty
     * 
     * @param lba Logical Block Address of the sector
     */
    void mark_dirty(uint32_t lba);

    /**
     * @brief Flush all dirty sectors to the physical ATA device
     * 
     * @param device Pointer to the ATA device
     */
    void flush(AtaDeviceInfo *device);
};
