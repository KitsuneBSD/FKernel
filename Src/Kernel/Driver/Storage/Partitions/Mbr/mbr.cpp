#include <Kernel/Driver/Storage/Partitions/Mbr/mbr.h>
#include <Kernel/Driver/Storage/Partitions/partition.h>
#include <Kernel/Driver/Storage/Partitions/partition_manager.h>
#include <Kernel/Driver/Storage/storage_device_name.h>
#include <LibFK/Algorithms/log.h>

fk::core::Result<void, fk::core::Error>
MBRParser::parse(fk::RefPtr<StorageDevice> device) {
  if (!device) return fk::core::Error::InvalidParameter;
  
  fk::algorithms::klog("MBR", "Scanning device '%s' for MBR...", device->name().c_str());

  size_t sector_size = device->sector_size().value();
  if (sector_size < sizeof(MBRHeader)) {
    fk::algorithms::kwarn("MBR", "Sector size too small for MBR header: %zu", sector_size);
    return fk::core::Error::InvalidData;
  }
  
  uint8_t* buffer = static_cast<uint8_t*>(kmalloc(sector_size));
  if (!buffer) return fk::core::Error::OutOfMemory;

  auto read_res = device->read_sectors(0, 1, buffer);
  if (read_res.is_error()) {
      fk::algorithms::kwarn("MBR", "Failed to read LBA 0");
      kfree(buffer);
      return read_res.error();
  }

  auto *mbr = reinterpret_cast<MBRHeader *>(buffer);
  if (!mbr || mbr->signature != 0xAA55) {
    fk::algorithms::kdebug("MBR", "No MBR signature (0xAA55) found at LBA 0 (Found: 0x%x)", mbr ? mbr->signature : 0);
    kfree(buffer);
    return fk::core::Error::InvalidData;
  }

  fk::algorithms::klog("MBR", "Valid MBR signature found");

  // Check if it's a protective MBR for GPT
  for (int i = 0; i < 4; ++i) {
      if (mbr->entries[i].system_id == 0xEE) {
          fk::algorithms::klog("MBR", "Protective MBR detected (GPT disk), skipping MBR parsing");
          kfree(buffer);
          return fk::core::Error::InvalidData; 
      }
  }

  int logical_count = 4;
  int found_partitions = 0;
  
  for (int i = 0; i < 4; ++i) {
    auto &entry = mbr->entries[i];
    if (entry.lba_length == 0 || entry.system_id == 0)
      continue;

    fk::algorithms::klog("MBR", "Found partition %d: Type 0x%02x, Start LBA %u, Length %u", 
                         i + 1, entry.system_id, entry.lba_start, entry.lba_length);

    if (entry.system_id == 0x05 || entry.system_id == 0x0F || entry.system_id == 0x85) {
      auto ext_res = parse_extended(device, entry.lba_start, entry.lba_start, logical_count);
      if (ext_res.is_error()) {
          fk::algorithms::kwarn("MBR", "Failed to parse extended partition at LBA %u", entry.lba_start);
      }
      continue;
    }

    fk::text::String name = StorageDeviceName::generate(device->name().c_str(), i + 1);
    auto partition_res = Partition::create(device, entry.lba_start, entry.lba_length, fk::types::move(name));
    if (partition_res.is_ok()) {
        PartitionManager::the().add_partition(partition_res.value());
        found_partitions++;
    }
  }

  fk::algorithms::klog("MBR", "MBR scan complete. Partitions found: %d", found_partitions);
  kfree(buffer);
  return (found_partitions > 0) ? fk::core::Result<void, fk::core::Error>({}) : fk::core::Error::NotFound;
}

fk::core::Result<void, fk::core::Error>
MBRParser::parse_extended(fk::RefPtr<StorageDevice> device,
                          uint32_t ebr_lba, uint32_t base_ebr_lba, int& partition_count) {
  size_t sector_size = device->sector_size().value();
  uint8_t* buffer = static_cast<uint8_t*>(kmalloc(sector_size));
  if (!buffer) return fk::core::Error::OutOfMemory;

  auto read_res = device->read_sectors(ebr_lba, 1, buffer);
  if (read_res.is_error()) {
      kfree(buffer);
      return read_res.error();
  }

  auto *mbr = reinterpret_cast<MBRHeader *>(buffer);
  if (mbr->signature != 0xAA55) {
    kfree(buffer);
    return fk::core::Error::InvalidData;
  }

  // First entry is the logical partition
  if (mbr->entries[0].lba_length > 0 && mbr->entries[0].system_id != 0) {
    fk::text::String name = StorageDeviceName::generate(device->name().c_str(), ++partition_count);
    auto partition_res = Partition::create(device, ebr_lba + mbr->entries[0].lba_start,
                                          mbr->entries[0].lba_length, fk::types::move(name));
    if (partition_res.is_ok()) {
        PartitionManager::the().add_partition(partition_res.value());
    }
  }

  // Second entry is the next EBR link (relative to base_ebr_lba)
  uint32_t next_ebr_lba = 0;
  if (mbr->entries[1].lba_length > 0 && (mbr->entries[1].system_id == 0x05 || mbr->entries[1].system_id == 0x0F || mbr->entries[1].system_id == 0x85)) {
      next_ebr_lba = base_ebr_lba + mbr->entries[1].lba_start;
  }

  kfree(buffer);

  if (next_ebr_lba != 0) {
      return parse_extended(device, next_ebr_lba, base_ebr_lba, partition_count);
  }

  return {};
}
