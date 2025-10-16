#pragma once

#include <LibC/stdint.h>

#include <Kernel/Driver/Ata/AtaController.h>

struct SectorCacheEntry
{
    uint32_t lba;
    bool is_valid;
    bool is_dirty;
    uint8_t data[512];
};

class AtaCache
{
private:
    AtaCache() = default;
    SectorCacheEntry cache[8];

public:
    static AtaCache &the();

    uint8_t *get_sector(AtaDeviceInfo *device, uint32_t lba);
    void mark_dirty(uint32_t lba);
    void flush(AtaDeviceInfo *device);
};