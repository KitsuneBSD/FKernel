#include <Kernel/Boot/Uefi/fat.h>
#include <Kernel/Boot/Uefi/block_io.h>
#include <Kernel/Boot/Uefi/uefi_types.h>
#include <LibFK/Core/Assertions.h>
#include <LibC/string.h>

namespace uefi {
namespace fat {

static bool read_sector(const FatContext &context, uint32_t sector, void *buffer) {
  return read_blocks(context.block_io,
                     context.block_io->Media->MediaId,
                     sector,
                     context.bytes_per_sector,
                     buffer) == EFI_SUCCESS;
}

static uint32_t cluster_to_sector(const FatContext &context, uint32_t cluster) {
  return context.first_data_sector + (cluster - 2) * context.sectors_per_cluster;
}

bool initialize(EFI_BLOCK_IO_PROTOCOL *block_io, FatContext &context) {
  if (!block_io || !block_io->Media) {
    return false;
  }

  context.block_io = block_io;
  
  // Allocate buffer for boot sector
  BootSector boot_sector;
  if (read_blocks(block_io, block_io->Media->MediaId, 0, sizeof(BootSector), &boot_sector) != EFI_SUCCESS) {
    return false;
  }

  context.boot_sector = &boot_sector;
  context.bytes_per_sector = boot_sector.bytes_per_sector;
  context.sectors_per_cluster = boot_sector.sectors_per_cluster;

  // Calculate FAT type
  uint32_t root_dir_sectors = ((boot_sector.root_entries * 32) + (boot_sector.bytes_per_sector - 1)) / boot_sector.bytes_per_sector;
  uint32_t fat_size = boot_sector.fat_size_16 ? boot_sector.fat_size_16 : boot_sector.fat_size_32;
  uint32_t total_sectors = boot_sector.total_sectors_16 ? boot_sector.total_sectors_16 : boot_sector.total_sectors_32;
  uint32_t data_sectors = total_sectors - (boot_sector.reserved_sectors + (boot_sector.num_fats * fat_size) + root_dir_sectors);
  uint32_t total_clusters = data_sectors / boot_sector.sectors_per_cluster;

  if (total_clusters < 4085) {
    context.fat_type = FatType::FAT12;
  } else if (total_clusters < 65525) {
    context.fat_type = FatType::FAT16;
  } else {
    context.fat_type = FatType::FAT32;
  }

  // Calculate important sector numbers
  context.first_fat_sector = boot_sector.reserved_sectors;
  context.first_data_sector = boot_sector.reserved_sectors + (boot_sector.num_fats * fat_size) + root_dir_sectors;

  if (context.fat_type == FatType::FAT32) {
    context.root_cluster = boot_sector.root_cluster;
    context.root_dir_sector = 0; // Not used for FAT32
  } else {
    context.root_cluster = 0; // Not used for FAT12/FAT16
    context.root_dir_sector = boot_sector.reserved_sectors + (boot_sector.num_fats * fat_size);
  }

  return true;
}

bool find_file(const FatContext &context, const char *filename, DirectoryEntry &entry) {
  // Convert filename to 8.3 format
  char name83[12] = {0};
  size_t len = strlen(filename);
  size_t dot_pos = len;
  for (size_t i = 0; i < len; ++i) {
    if (filename[i] == '.') {
      dot_pos = i;
      break;
    }
  }

  // Copy name (up to 8 chars)
  size_t name_len = dot_pos < 8 ? dot_pos : 8;
  for (size_t i = 0; i < name_len; ++i) {
    name83[i] = (filename[i] >= 'a' && filename[i] <= 'z') ? filename[i] - 32 : filename[i];
  }
  for (size_t i = name_len; i < 8; ++i) {
    name83[i] = ' ';
  }

  // Copy extension (up to 3 chars)
  if (dot_pos < len) {
    size_t ext_start = dot_pos + 1;
    size_t ext_len = len - ext_start;
    if (ext_len > 3) ext_len = 3;
    for (size_t i = 0; i < ext_len; ++i) {
      name83[8 + i] = (filename[ext_start + i] >= 'a' && filename[ext_start + i] <= 'z') 
                      ? filename[ext_start + i] - 32 : filename[ext_start + i];
    }
  }

  // Read root directory
  uint32_t root_sector = context.root_dir_sector;
  if (context.fat_type == FatType::FAT32) {
    root_sector = cluster_to_sector(context, context.root_cluster);
  }

  uint8_t sector_buffer[512];
  for (uint32_t i = 0; i < context.root_dir_sectors || (context.fat_type == FatType::FAT32 && i < context.sectors_per_cluster); ++i) {
    if (!read_sector(context, root_sector + i, sector_buffer)) {
      continue;
    }

    // Search directory entries in this sector
    for (size_t j = 0; j < context.bytes_per_sector; j += sizeof(DirectoryEntry)) {
      DirectoryEntry *dir_entry = reinterpret_cast<DirectoryEntry *>(&sector_buffer[j]);
      
      // Check if entry is valid (not deleted, not volume label)
      if (dir_entry->name[0] == 0) {
        return false; // End of directory
      }
      if (static_cast<uint8_t>(dir_entry->name[0]) == 0xE5 || dir_entry->attributes == 0x0F) {
        continue; // Deleted entry or LFN entry
      }

      // Compare names
      if (memcmp(dir_entry->name, name83, 11) == 0) {
        entry = *dir_entry;
        return true;
      }
    }

    if (context.fat_type != FatType::FAT32) {
      break; // FAT12/FAT16 root directory is fixed size
    }
  }

  return false;
}

bool read_cluster(const FatContext &context, uint32_t cluster, void *buffer) {
  uint32_t sector = cluster_to_sector(context, cluster);
  return read_sector(context, sector, buffer);
}

uint32_t get_next_cluster(const FatContext &context, uint32_t cluster) {
  uint32_t fat_offset;
  uint32_t fat_sector;
  uint32_t ent_offset;
  uint8_t fat_sector_buffer[512];

  if (context.fat_type == FatType::FAT32) {
    fat_offset = cluster * 4;
    fat_sector = context.first_fat_sector + (fat_offset / context.bytes_per_sector);
    ent_offset = fat_offset % context.bytes_per_sector;

    if (!read_sector(context, fat_sector, fat_sector_buffer)) {
      return 0xFFFFFFFF;
    }

    uint32_t *next_cluster = reinterpret_cast<uint32_t *>(&fat_sector_buffer[ent_offset]);
    return (*next_cluster) & 0x0FFFFFFF;
  } else if (context.fat_type == FatType::FAT16) {
    fat_offset = cluster * 2;
    fat_sector = context.first_fat_sector + (fat_offset / context.bytes_per_sector);
    ent_offset = fat_offset % context.bytes_per_sector;

    if (!read_sector(context, fat_sector, fat_sector_buffer)) {
      return 0xFFFF;
    }

    uint16_t *next_cluster = reinterpret_cast<uint16_t *>(&fat_sector_buffer[ent_offset]);
    return *next_cluster;
  } else { // FAT12
    fat_offset = cluster + (cluster / 2);
    fat_sector = context.first_fat_sector + (fat_offset / context.bytes_per_sector);
    ent_offset = fat_offset % context.bytes_per_sector;

    if (!read_sector(context, fat_sector, fat_sector_buffer)) {
      return 0xFFF;
    }

    uint16_t next = *reinterpret_cast<uint16_t *>(&fat_sector_buffer[ent_offset]);
    if (cluster & 1) {
      return (next >> 4) & 0xFFF;
    } else {
      return next & 0xFFF;
    }
  }
}

bool read_file(const FatContext &context, const char *filename, void *buffer, size_t buffer_size, size_t &bytes_read) {
  DirectoryEntry entry;
  if (!find_file(context, filename, entry)) {
    return false;
  }

  uint32_t cluster;
  if (context.fat_type == FatType::FAT32) {
    cluster = entry.first_cluster_low | (entry.first_cluster_high << 16);
  } else {
    cluster = entry.first_cluster_low;
  }

  bytes_read = 0;
  size_t file_size = entry.file_size;
  if (file_size > buffer_size) {
    file_size = buffer_size;
  }

  uint8_t *buf = reinterpret_cast<uint8_t *>(buffer);
  uint32_t cluster_size = context.bytes_per_sector * context.sectors_per_cluster;

  while (bytes_read < file_size && cluster != 0 && cluster < 0xFFFFFFF8) {
    uint8_t cluster_buffer[4096]; // Max cluster size for 4KB sectors
    if (cluster_size > sizeof(cluster_buffer)) {
      return false; // Cluster too large
    }

    if (!read_cluster(context, cluster, cluster_buffer)) {
      return false;
    }

    size_t to_copy = file_size - bytes_read;
    if (to_copy > cluster_size) {
      to_copy = cluster_size;
    }

    memcpy(buf + bytes_read, cluster_buffer, to_copy);
    bytes_read += to_copy;

    cluster = get_next_cluster(context, cluster);
    if (cluster >= 0xFFFFFFF8) {
      break; // End of file
    }
  }

  return true;
}

} // namespace fat
} // namespace uefi
