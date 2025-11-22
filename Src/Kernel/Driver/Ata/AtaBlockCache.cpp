#include <Kernel/Block/BlockDevice.h>
#include <Kernel/Driver/Ata/AtaBlockCache.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

AtaCache &AtaCache::the() {
  static AtaCache inst;
  return inst;
}

uint8_t *AtaCache::get_sector(const BlockDevice *device, uint32_t lba) {
  for (auto &entry : cache) {
    if (entry.is_valid && entry.lba == lba) {
      return entry.data;
    }
  }

  SectorCacheEntry *slot = &cache[0];
  for (auto &entry : cache) {
    if (!entry.is_valid) {
      slot = &entry;
      break;
    }
  }

  device->read_sectors(lba, 1, slot->data);
  slot->lba = lba;
  slot->is_valid = true;
  slot->is_dirty = false;
  return slot->data;
}


void AtaCache::mark_dirty(uint32_t lba) {
  for (auto &entry : cache) {
    if (entry.is_valid && entry.lba == lba) {
      entry.is_dirty = true;
      return;
    }
  }
}

void AtaCache::flush(BlockDevice *device) {
  for (auto &entry : cache) {
    if (entry.is_valid && entry.is_dirty) {
      device->write_sectors(entry.lba, 1, entry.data);
      entry.is_dirty = false;
    }
  }
}
