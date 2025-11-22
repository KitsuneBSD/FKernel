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
      fk::algorithms::kdebug("ATA CACHE", "Cache HIT for LBA %u.", lba);
      return entry.data;
    }
  }

  fk::algorithms::kdebug("ATA CACHE",
                         "Cache MISS for LBA %u. Reading from device.", lba);
  SectorCacheEntry *slot = &cache[0]; // Simple LRU (or FIFO) replacement, just
                                      // take the first invalid/oldest slot
  for (auto &entry : cache) {
    if (!entry.is_valid) {
      slot = &entry;
      break;
    }
  }

  int sectors_read = device->read_sectors(lba, 1, slot->data);
  if (sectors_read != 1) {
    fk::algorithms::kerror("ATA CACHE",
                           "Failed to read sector %u from device. Result: %d",
                           lba, sectors_read);
    // Depending on error handling policy, may return nullptr or a dummy buffer
    return nullptr; // Indicate failure
  }

  slot->lba = lba;
  slot->is_valid = true;
  slot->is_dirty = false;
  fk::algorithms::kdebug("ATA CACHE", "Sector %u successfully read into cache.",
                         lba);
  return slot->data;
}

void AtaCache::mark_dirty(uint32_t lba) {
  for (auto &entry : cache) {
    if (entry.is_valid && entry.lba == lba) {
      entry.is_dirty = true;
      fk::algorithms::kdebug("ATA CACHE", "LBA %u marked as dirty.", lba);
      return;
    }
  }
  fk::algorithms::kwarn(
      "ATA CACHE", "Attempted to mark non-cached LBA %u as dirty. Ignoring.",
      lba);
}

void AtaCache::flush(BlockDevice *device) {
  fk::algorithms::klog("ATA CACHE", "Flushing dirty cache entries.");
  for (auto &entry : cache) {
    if (entry.is_valid && entry.is_dirty) {
      fk::algorithms::kdebug("ATA CACHE", "Flushing dirty LBA %u.", entry.lba);
      int sectors_written = device->write_sectors(entry.lba, 1, entry.data);
      if (sectors_written != 1) {
        fk::algorithms::kerror("ATA CACHE",
                               "Failed to flush LBA %u. Result: %d", entry.lba,
                               sectors_written);
      } else {
        entry.is_dirty = false;
        fk::algorithms::kdebug("ATA CACHE", "LBA %u successfully flushed.",
                               entry.lba);
      }
    }
  }
  fk::algorithms::klog("ATA CACHE", "Finished flushing cache.");
}
