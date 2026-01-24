#include <Kernel/Driver/Storage/Partitions/Gpt/gpt.h>
#include <Kernel/Driver/Storage/Partitions/partition.h>
#include <Kernel/Driver/Storage/Partitions/partition_manager.h>
#include <Kernel/Driver/Storage/storage_device_name.h>
#include <LibFK/Utilities/Memory.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Algorithms/crc32.h>
#include <LibFK/Memory/heap_malloc.h>

fk::core::Result<void, fk::core::Error>
GPTParser::parse(fk::RefPtr<StorageDevice> device) {
  if (!device) return fk::core::Error::InvalidParameter;
  
  size_t sector_size = device->sector_size().value();
  if (sector_size < sizeof(GPTHeader)) {
    fk::algorithms::kwarn("GPT", "Sector size too small for GPT header: %zu", sector_size);
    return fk::core::Error::InvalidData;
  }
  
  uint8_t* buffer = static_cast<uint8_t*>(kmalloc(sector_size));
  if (!buffer) return fk::core::Error::OutOfMemory;

  auto read_res = device->read_sectors(1, 1, buffer);
  bool primary_ok = false;
  if (read_res.is_ok()) {
    auto *header = reinterpret_cast<GPTHeader *>(buffer);
    // Verify header is within buffer bounds
    if (header->header_size > 0 && header->header_size <= sector_size) {
      fk::algorithms::kdebug("GPT", "Primary header signature: %c%c%c%c%c%c%c%c%c", 
                             header->signature[0], header->signature[1], header->signature[2],
                             header->signature[3], header->signature[4], header->signature[5],
                             header->signature[6], header->signature[7]);
      if (fk::memory::compare(header->signature, "EFI PART", 8) == 0) {
        uint32_t original_crc = header->header_crc;
        header->header_crc = 0;
        uint32_t calculated_crc = fk::algorithms::crc32(header, header->header_size);
        header->header_crc = original_crc;
        fk::algorithms::kdebug("GPT", "Primary CRC: expected=%08x, calculated=%08x", 
                               original_crc, calculated_crc);
        if (calculated_crc == original_crc) primary_ok = true;
      } else {
        fk::algorithms::kwarn("GPT", "Invalid signature, not EFI PART");
      }
    } else {
      fk::algorithms::kwarn("GPT", "Invalid header size: %u", header->header_size);
    }
  } else {
    fk::algorithms::kwarn("GPT", "Failed to read primary header LBA 1");
  }

  if (!primary_ok) {
    fk::algorithms::kwarn("GPT", "Primary header corrupted, trying backup...");
   uint64_t total_sectors = device->sector_count().value();
   
   // Validate sector count before using it
   if (total_sectors == 0 || total_sectors > 0x100000000ULL) {  // Reasonable upper limit
     fk::algorithms::kwarn("GPT", "Invalid total sectors: %llu", total_sectors);
     kfree(buffer);
     return fk::core::Error::InvalidData;
   }
   
   uint64_t last_lba = total_sectors - 1;
   fk::algorithms::kdebug("GPT", "Total sectors: %llu, trying backup at LBA: %llu", total_sectors, last_lba);
   
   if (last_lba == 0) {
     kfree(buffer);
     return fk::core::Error::InvalidData;
   }
    
    if (device->read_sectors(last_lba, 1, buffer).is_error()) {
      kfree(buffer);
      return fk::core::Error::IOError;
    }
    auto *header = reinterpret_cast<GPTHeader *>(buffer);
    if (fk::memory::compare(header->signature, "EFI PART", 8) != 0) {
      kfree(buffer);
      return fk::core::Error::InvalidData;
    }
    
    // Validate backup header size before CRC
    if (header->header_size == 0 || header->header_size > sector_size) {
      fk::algorithms::kwarn("GPT", "Backup header has invalid size: %u", header->header_size);
      kfree(buffer);
      return fk::core::Error::InvalidData;
    }
    
    // Validate backup header CRC32 too
    uint32_t backup_original_crc = header->header_crc;
    header->header_crc = 0;
    uint32_t backup_calculated_crc = fk::algorithms::crc32(header, header->header_size);
    header->header_crc = backup_original_crc;
    
    if (backup_calculated_crc != backup_original_crc) {
      fk::algorithms::kwarn("GPT", "Backup header CRC32 mismatch too");
      kfree(buffer);
      return fk::core::Error::InvalidData;
    }
  }

  auto *header = reinterpret_cast<GPTHeader *>(buffer);
  if (header->header_size < sizeof(GPTHeader) || header->header_size > sector_size) {
    fk::algorithms::kwarn("GPT", "Invalid header size: %u", header->header_size);
    kfree(buffer);
    return fk::core::Error::InvalidData;
  }

  // Validate Header CRC32
  uint32_t original_crc = header->header_crc;
  header->header_crc = 0;
  uint32_t calculated_crc = fk::algorithms::crc32(header, header->header_size);
  header->header_crc = original_crc;

  if (calculated_crc != original_crc) {
    fk::algorithms::kwarn("GPT", "Header CRC32 mismatch! (Exp: %08x, Got: %08x)", original_crc, calculated_crc);
    kfree(buffer);
    return fk::core::Error::InvalidData;
  }

  size_t entries_size = header->num_partition_entries * header->partition_entry_size;
  
  if (header->partition_entry_size < 128 || header->partition_entry_size > 4096) {
      fk::algorithms::kwarn("GPT", "Invalid partition entry size: %u", header->partition_entry_size);
      kfree(buffer);
      return fk::core::Error::InvalidData;
  }

  if (header->num_partition_entries > 1024) {
      fk::algorithms::kwarn("GPT", "Too many partition entries: %u", header->num_partition_entries);
      kfree(buffer);
      return fk::core::Error::InvalidData;
  }

  size_t sectors_to_read = (entries_size + sector_size - 1) / sector_size;
  fk::algorithms::kdebug("GPT", "Reading %zu sectors for %u partition entries", sectors_to_read, header->num_partition_entries);

  uint8_t *entries_buffer = static_cast<uint8_t *>(kmalloc(sectors_to_read * sector_size));
  if (!entries_buffer) {
    kfree(buffer);
    return fk::core::Error::OutOfMemory;
  }

  auto entries_read_res = device->read_sectors(header->partition_entries_lba,
                                     sectors_to_read, entries_buffer);
  if (entries_read_res.is_error()) {
    kfree(entries_buffer);
    kfree(buffer);
    return entries_read_res.error();
  }

  // Validate Partition Entries CRC32
  uint32_t entries_crc = fk::algorithms::crc32(entries_buffer, entries_size);
  if (entries_crc != header->partition_entries_crc) {
    fk::algorithms::kwarn("GPT", "Partition entries CRC32 mismatch!");
    kfree(entries_buffer);
    kfree(buffer);
    return fk::core::Error::InvalidData;
  }

  uint32_t partition_count = 0;
  for (uint32_t i = 0; i < header->num_partition_entries; ++i) {
    auto *entry = reinterpret_cast<GPTPartitionEntry *>(
        entries_buffer + (i * header->partition_entry_size));

    // Skip empty entries (Type GUID all zeros)
    static const uint8_t zero_guid[16] = {0};
    if (fk::memory::compare(entry->partition_type_guid, zero_guid, 16) == 0)
        continue;

    // Validate LBA range
    if (entry->ending_lba < entry->starting_lba) {
        fk::algorithms::kwarn("GPT", "Partition %u has invalid LBA range", i);
        continue;
    }

    // Ensure partition is within usable disk area
    if (entry->starting_lba < header->first_usable_lba || entry->ending_lba > header->last_usable_lba) {
        fk::algorithms::kwarn("GPT", "Partition %u is outside usable area (start: %llu, end: %llu, first_usable: %llu, last_usable: %llu)", 
                             i, entry->starting_lba, entry->ending_lba, header->first_usable_lba, header->last_usable_lba);
        continue;
    }
    
    // Additional validation: ensure starting LBA is reasonable and not zero
    if (entry->starting_lba == 0 || entry->starting_lba >= device->sector_count().value()) {
        fk::algorithms::kwarn("GPT", "Partition %u has invalid starting LBA: %llu", i, entry->starting_lba);
        continue;
    }
    
    // Ensure count is reasonable
    uint64_t count = entry->ending_lba - entry->starting_lba + 1;
    if (count == 0 || count > device->sector_count().value()) {
        fk::algorithms::kwarn("GPT", "Partition %u has invalid count: %llu", i, count);
        continue;
    }
    
    // Log partition name (UTF-16LE to ASCII conversion) with bounds checking
    char name_buffer[37] = {0};  // Zero-initialize
    for (int n = 0; n < 36; ++n) {
        uint16_t wchar = entry->partition_name[n];
        if (wchar == 0) break;
        name_buffer[n] = (wchar < 128) ? static_cast<char>(wchar) : '?';
    }
    name_buffer[36] = '\0';

    static const uint8_t esp_guid[16] = {0x28, 0x73, 0x2A, 0xC1, 0x1F, 0xF8, 0xD2, 0x11, 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B};
    bool is_esp = (fk::memory::compare(entry->partition_type_guid, esp_guid, 16) == 0);

    fk::algorithms::kdebug("GPT", "Partition %u: %sName='%s', Start=%llu, Count=%llu", 
                           i + 1, is_esp ? "[ESP] " : "", name_buffer, entry->starting_lba, count);

    // BSD/Linux style naming: ad0p1, ad0p2, etc. where the number is the 1-based index in the table.
    // However, FKernel's StorageDeviceName::generate just appends. 
    // We'll use i + 1 to maintain consistency with entry indices.
    fk::text::String name = StorageDeviceName::generate(device->name().c_str(), i + 1);
    
    auto partition_res = Partition::create(device, entry->starting_lba, count, fk::types::move(name));
    if (partition_res.is_ok()) {
        PartitionManager::the().add_partition(partition_res.value());
        partition_count++;
    }
  }
  
  fk::algorithms::klog("GPT", "Detected %u valid partitions", partition_count);

  kfree(entries_buffer);
  kfree(buffer);
  
  // If no valid partitions were found, return error to allow MBR fallback
  if (partition_count == 0) {
    return fk::core::Error::InvalidData;
  }
  
  return {};
}
