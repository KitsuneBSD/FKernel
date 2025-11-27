#include <Kernel/Block/PartitionDevice.h> // Direct include for fkernel::block::PartitionBlockDevice
#include <Kernel/FileSystem/Fat/fat_fs.h>
#include <LibC/ctype.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/own_ptr.h>

namespace fkernel::fs::fat {

static_assert(fk::traits::is_base_of_v<::BlockDevice,
                                       fkernel::block::PartitionBlockDevice>,
              "PartitionBlockDevice must inherit from BlockDevice!");

// Static VNodeOps table definition
VNodeOps FatFileSystem::s_fat_vnode_ops = {
    .read = &FatFileSystem::fat_read,
    .write = &FatFileSystem::fat_write,
    .open = &FatFileSystem::fat_open,
    .close = &FatFileSystem::fat_close,
    .lookup = &FatFileSystem::fat_lookup,
    .create = &FatFileSystem::fat_create,
    .readdir = &FatFileSystem::fat_readdir,
    .unlink = &FatFileSystem::fat_unlink,
};

FatFileSystem::FatFileSystem(
    fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice> block_device,
    uint32_t first_sector)
    : m_block_device(static_cast<BlockDevice *>(block_device.get())),
      m_first_sector(first_sector) {
  memset(&m_boot_sector, 0, sizeof(FatBootSector));
  fk::algorithms::klog("FAT FS", "FatFileSystem constructed for first sector %u.",
                       first_sector);
}

fk::memory::RetainPtr<VNode> FatFileSystem::root_vnode() {
  return m_root_vnode;
}

int FatFileSystem::initialize() {
  fk::algorithms::klog("FAT FS", "Initializing FAT filesystem...");

  int result = read_boot_sector(&m_boot_sector);
  if (result < 0) {
    fk::algorithms::kerror("FAT FS", "Failed to read boot sector.");
    return result;
  }

  // Validate boot sector signature
  if (m_boot_sector.boot_sector_signature != 0xAA55) {
    fk::algorithms::kerror("FAT FS", "Invalid boot sector signature: 0x%x.",
                           m_boot_sector.boot_sector_signature);
    return -1;
  }

  m_fat_type = determine_fat_type();
  if (m_fat_type == FatType::Unknown) {
    fk::algorithms::kerror("FAT FS", "Unknown FAT type.");
    return -1;
  }

  fk::algorithms::klog(
      "FAT FS", "Detected FAT type: %s",
      m_fat_type == FatType::FAT12
          ? "FAT12"
          : (m_fat_type == FatType::FAT16 ? "FAT16" : "FAT32"));
  fk::algorithms::klog("FAT FS", "Bytes per sector: %u",
                       m_boot_sector.bytes_per_sector);
  fk::algorithms::klog("FAT FS", "Sectors per cluster: %u",
                       m_boot_sector.sectors_per_cluster);
  fk::algorithms::klog("FAT FS", "Reserved sectors: %u",
                       m_boot_sector.reserved_sectors);
  fk::algorithms::klog("FAT FS", "Number of FATs: %u", m_boot_sector.fat_count);

  m_sectors_per_fat = get_sectors_per_fat();
  m_fat_start_sector = m_first_sector + m_boot_sector.reserved_sectors;

  if (m_fat_type == FatType::FAT32) {
    m_root_dir_start_sector =
        m_fat_start_sector + (m_boot_sector.fat_count * m_sectors_per_fat);
    m_data_start_sector = m_root_dir_start_sector;
  } else {
    m_root_dir_start_sector =
        m_fat_start_sector + (m_boot_sector.fat_count * m_sectors_per_fat);
    m_data_start_sector = m_root_dir_start_sector + get_root_dir_sectors();
  }

  m_total_clusters = get_total_clusters();

  fk::algorithms::klog("FAT FS", "Sectors per FAT: %u", m_sectors_per_fat);
  fk::algorithms::klog("FAT FS", "FAT start sector: %u", m_fat_start_sector);
  fk::algorithms::klog("FAT FS", "Root Dir start sector: %u",
                       m_root_dir_start_sector);
  fk::algorithms::klog("FAT FS", "Data start sector: %u", m_data_start_sector);
  fk::algorithms::klog("FAT FS", "Total clusters: %u", m_total_clusters);

  // Initialize the root VNode
  auto fat_root_vnode = fk::memory::adopt_retain(new VNode());
  fat_root_vnode->m_name = "/"; // Root of this filesystem
  fat_root_vnode->type = VNodeType::Directory;
  fat_root_vnode->ops = &fkernel::fs::fat::FatFileSystem::s_fat_vnode_ops;
  fat_root_vnode->fs_private =
      this; // Point back to this FatFileSystem instance

  if (m_fat_type == fkernel::fs::fat::FatType::FAT32) {
    fat_root_vnode->inode_number = m_boot_sector.root_cluster;
    fkernel::fs::fat::FatFilePrivateData *root_dir_data =
        new fkernel::fs::fat::FatFilePrivateData();
    root_dir_data->first_cluster = m_boot_sector.root_cluster;
    root_dir_data->current_offset = 0;
    root_dir_data->size = 0;
    fat_root_vnode->inode = new Inode(m_boot_sector.root_cluster);
    fat_root_vnode->inode->data_block_pointers[0] =
        reinterpret_cast<uint64_t>(root_dir_data);
  } else {
    fat_root_vnode->inode_number = 0;
    fat_root_vnode->inode = new Inode(0);
  }
  m_root_vnode = fat_root_vnode;
  fk::algorithms::klog("FAT FS", "Root VNode initialized.");

  return 0;
}

int FatFileSystem::read_boot_sector(FatBootSector *boot_sector) const {
  fk::algorithms::kdebug("FAT FS", "Reading boot sector from LBA %u.", m_first_sector);
  if (!m_block_device) {
    fk::algorithms::kerror("FAT FS", "Block device is null.");
    return -1;
  }
  if (!boot_sector) {
    fk::algorithms::kerror("FAT FS", "Boot sector buffer is null.");
    return -1;
  }

  // Read the first sector (boot sector) of the partition
  int sectors_read =
      m_block_device->read_sectors(m_first_sector, 1, boot_sector);
  if (sectors_read != 1) {
    fk::algorithms::kerror("FAT FS", "Failed to read boot sector from LBA %u.",
                           m_first_sector);
    return -1;
  }
  return 0;
}

FatType FatFileSystem::determine_fat_type() const {
  uint32_t total_sectors = m_boot_sector.total_sectors_16 == 0
                               ? m_boot_sector.total_sectors_32
                               : m_boot_sector.total_sectors_16;

  uint32_t fat_size = m_boot_sector.sectors_per_fat_16 == 0
                          ? m_boot_sector.sectors_per_fat_32
                          : m_boot_sector.sectors_per_fat_16;

  uint32_t root_dir_sectors = ((m_boot_sector.root_dir_entries * 32) +
                               (m_boot_sector.bytes_per_sector - 1)) /
                              m_boot_sector.bytes_per_sector;

  uint32_t data_sectors =
      total_sectors - (m_boot_sector.reserved_sectors +
                       (m_boot_sector.fat_count * fat_size) + root_dir_sectors);

  uint32_t total_clusters = data_sectors / m_boot_sector.sectors_per_cluster;

  if (total_clusters < 4085) {
    return FatType::FAT12;
  } else if (total_clusters < 65525) {
    return FatType::FAT16;
  } else {
    return FatType::FAT32;
  }
}

uint32_t FatFileSystem::get_sectors_per_fat() const {
  if (m_fat_type == FatType::FAT32) {
    return m_boot_sector.sectors_per_fat_32;
  } else {
    return m_boot_sector.sectors_per_fat_16;
  }
}

uint32_t FatFileSystem::get_root_dir_sectors() const {
  return ((m_boot_sector.root_dir_entries * sizeof(FatDirEntry)) +
          (m_boot_sector.bytes_per_sector - 1)) /
         m_boot_sector.bytes_per_sector;
}

uint32_t FatFileSystem::get_total_clusters() const {
  uint32_t total_sectors = m_boot_sector.total_sectors_16 == 0
                               ? m_boot_sector.total_sectors_32
                               : m_boot_sector.total_sectors_16;

  uint32_t fat_size = get_sectors_per_fat();

  uint32_t root_dir_sectors = get_root_dir_sectors();

  uint32_t data_sectors =
      total_sectors - (m_boot_sector.reserved_sectors +
                       (m_boot_sector.fat_count * fat_size) + root_dir_sectors);
  return data_sectors / m_boot_sector.sectors_per_cluster;
}

uint32_t FatFileSystem::get_first_fat_sector() const {
  return m_first_sector + m_boot_sector.reserved_sectors;
}

uint32_t FatFileSystem::cluster_to_lba(uint32_t cluster) const {
  // For FAT12/16, the data region starts after boot, reserved, FATs, and root
  // directory. For FAT32, the root directory is just another cluster chain, so
  // data region starts after boot, reserved, and FATs.
  uint32_t lba =
      m_data_start_sector + (cluster - 2) * m_boot_sector.sectors_per_cluster;
  return lba;
}

int FatFileSystem::read_cluster(uint32_t cluster, void *buffer) const {
  if (!m_block_device) {
    fk::algorithms::kerror("FAT FS", "Block device is null.");
    return -1;
  }
  if (!buffer) {
    fk::algorithms::kerror("FAT FS", "Buffer is null.");
    return -1;
  }

  uint32_t lba = cluster_to_lba(cluster);
  uint8_t sectors_to_read = m_boot_sector.sectors_per_cluster;

  fk::algorithms::kdebug("FAT FS", "Reading cluster %u (LBA %u, %u sectors)",
                         cluster, lba, sectors_to_read);

  int sectors_read = m_block_device->read_sectors(lba, sectors_to_read, buffer);
  if (sectors_read != sectors_to_read) {
    fk::algorithms::kerror("FAT FS", "Failed to read cluster %u (LBA %u).",
                           cluster, lba);
    return -1;
  }
  return 0;
}

int FatFileSystem::write_cluster(uint32_t cluster, const void *buffer) {
  if (!m_block_device) {
    fk::algorithms::kerror("FAT FS", "Block device is null.");
    return -1;
  }
  if (!buffer) {
    fk::algorithms::kerror("FAT FS", "Buffer is null.");
    return -1;
  }

  uint32_t lba = cluster_to_lba(cluster);
  uint8_t sectors_to_write = m_boot_sector.sectors_per_cluster;

  fk::algorithms::kdebug("FAT FS", "Writing cluster %u (LBA %u, %u sectors)",
                         cluster, lba, sectors_to_write);

  int sectors_written =
      m_block_device->write_sectors(lba, sectors_to_write, buffer);
  if (sectors_written != sectors_to_write) {
    fk::algorithms::kerror("FAT FS", "Failed to write cluster %u (LBA %u).",
                           cluster, lba);
    return -1;
  }
  return 0;
}

uint32_t FatFileSystem::get_next_cluster(uint32_t current_cluster) const {
  fk::algorithms::kdebug("FAT FS", "Getting next cluster for %u.", current_cluster);
  uint32_t fat_offset;
  uint32_t fat_sector;
  uint32_t entry_value;

  switch (m_fat_type) {
  case FatType::FAT12:
    fat_offset = current_cluster + (current_cluster / 2);
    break;
  case FatType::FAT16:
    fat_offset = current_cluster * 2;
    break;
  case FatType::FAT32:
    fat_offset = current_cluster * 4;
    break;
  case FatType::Unknown:
    return 0; // Error
  }

  fat_sector =
      m_fat_start_sector + (fat_offset / m_boot_sector.bytes_per_sector);
  uint32_t sector_offset = fat_offset % m_boot_sector.bytes_per_sector;

  alignas(16) uint8_t sector_data[MAX_SECTOR_SIZE];
  if (m_block_device->read_sectors(fat_sector, 1, sector_data) != 1) {
    fk::algorithms::kerror("FAT FS", "Failed to read FAT sector %u.",
                           fat_sector);
    return 0;
  }

  switch (m_fat_type) {
  case FatType::FAT12:
    entry_value = *reinterpret_cast<uint16_t *>(&sector_data[sector_offset]);
    if (current_cluster & 0x0001) {
      entry_value >>= 4;
    } else {
      entry_value &= 0x0FFF;
    }
    if (entry_value >= 0x0FF8)
      return 0xFFFFFFFF; // EOF
    if (entry_value == 0x0FF7)
      return 0xFFFFFFF7; // Bad cluster
    break;
  case FatType::FAT16:
    entry_value = *reinterpret_cast<uint16_t *>(&sector_data[sector_offset]);
    if (entry_value >= 0xFFF8)
      return 0xFFFFFFFF; // EOF
    if (entry_value == 0xFFF7)
      return 0xFFFFFFF7; // Bad cluster
    break;
  case FatType::FAT32:
    entry_value = *reinterpret_cast<uint32_t *>(&sector_data[sector_offset]) &
                  0x0FFFFFFF; // FAT32 uses 28 bits
    if (entry_value >= 0x0FFFFFF8)
      return 0xFFFFFFFF; // EOF
    if (entry_value == 0x0FFFFFF7)
      return 0xFFFFFFF7; // Bad cluster
    break;
  case FatType::Unknown:
    return 0; // Error
  }
  return entry_value;
}

int FatFileSystem::set_next_cluster(uint32_t current_cluster,
                                    uint32_t next_cluster) {
  uint32_t fat_offset;
  uint32_t fat_sector;

  switch (m_fat_type) {
  case FatType::FAT12:
    fat_offset = current_cluster + (current_cluster / 2);
    break;
  case FatType::FAT16:
    fat_offset = current_cluster * 2;
    break;
  case FatType::FAT32:
    fat_offset = current_cluster * 4;
    break;
  case FatType::Unknown:
    return -1; // Error
  }

  fat_sector =
      m_fat_start_sector + (fat_offset / m_boot_sector.bytes_per_sector);
  uint32_t sector_offset = fat_offset % m_boot_sector.bytes_per_sector;

  alignas(16) uint8_t sector_data[MAX_SECTOR_SIZE];
  if (m_block_device->read_sectors(fat_sector, 1, sector_data) != 1) {
    fk::algorithms::kerror("FAT FS", "Failed to read FAT sector %u for write.",
                           fat_sector);
    return -1;
  }

  switch (m_fat_type) {
  case FatType::FAT12: {
    uint16_t *entry = reinterpret_cast<uint16_t *>(&sector_data[sector_offset]);
    if (current_cluster & 0x0001) {
      *entry = (*entry & 0x000F) | (next_cluster << 4);
    } else {
      *entry = (*entry & 0xF000) | (next_cluster & 0x0FFF);
    }
    break;
  }
  case FatType::FAT16:
    *reinterpret_cast<uint16_t *>(&sector_data[sector_offset]) =
        static_cast<uint16_t>(next_cluster);
    break;
  case FatType::FAT32:
    *reinterpret_cast<uint32_t *>(&sector_data[sector_offset]) =
        (*reinterpret_cast<uint32_t *>(&sector_data[sector_offset]) &
         0xF0000000) |
        (next_cluster & 0x0FFFFFFF);
    break;
  case FatType::Unknown:
    return -1; // Error
  }

  if (m_block_device->write_sectors(fat_sector, 1, sector_data) != 1) {
    fk::algorithms::kerror("FAT FS", "Failed to write FAT sector %u.",
                           fat_sector);
    return -1;
  }

  // Write to second FAT table as well
  if (m_boot_sector.fat_count > 1) {
    fat_sector += m_sectors_per_fat;
    if (m_block_device->write_sectors(fat_sector, 1, sector_data) != 1) {
      fk::algorithms::kerror(
          "FAT FS", "Failed to write to second FAT sector %u.", fat_sector);
      return -1;
    }
  }

  return 0;
}

uint32_t FatFileSystem::find_free_cluster() const {
  fk::algorithms::kdebug("FAT FS", "Searching for a free cluster.");
  // Start searching from cluster 2 (first data cluster)
  for (uint32_t cluster = 2; cluster < m_total_clusters + 2; ++cluster) {
    uint32_t fat_offset;
    uint32_t fat_sector;
    uint32_t entry_value;

    switch (m_fat_type) {
    case FatType::FAT12:
      fat_offset = cluster + (cluster / 2);
      break;
    case FatType::FAT16:
      fat_offset = cluster * 2;
      break;
    case FatType::FAT32:
      fat_offset = cluster * 4;
      break;
    case FatType::Unknown:
      return 0; // Error
    }

    fat_sector =
        m_fat_start_sector + (fat_offset / m_boot_sector.bytes_per_sector);
    uint32_t sector_offset = fat_offset % m_boot_sector.bytes_per_sector;

    alignas(16) uint8_t sector_data[MAX_SECTOR_SIZE];
    if (m_block_device->read_sectors(fat_sector, 1, sector_data) != 1) {
      fk::algorithms::kerror(
          "FAT FS", "Failed to read FAT sector %u during free cluster search.",
          fat_sector);
      return 0;
    }

    switch (m_fat_type) {
    case FatType::FAT12:
      entry_value = *reinterpret_cast<uint16_t *>(&sector_data[sector_offset]);
      if (cluster & 0x0001) {
        entry_value >>= 4;
      } else {
        entry_value &= 0x0FFF;
      }
      if (entry_value == 0x000)
        return cluster;
      break;
    case FatType::FAT16:
      entry_value = *reinterpret_cast<uint16_t *>(&sector_data[sector_offset]);
      if (entry_value == 0x0000)
        return cluster;
      break;
    case FatType::FAT32:
      entry_value = *reinterpret_cast<uint32_t *>(&sector_data[sector_offset]) &
                    0x0FFFFFFF;
      if (entry_value == 0x00000000)
        return cluster;
      break;
    case FatType::Unknown:
      return 0; // Error
    }
  }
  fk::algorithms::kwarn("FAT FS", "No free clusters found.");
  return 0;
}

uint32_t FatFileSystem::allocate_cluster(uint32_t previous_cluster) {
  uint32_t free_cluster = find_free_cluster();
  if (free_cluster == 0) {
    fk::algorithms::kerror("FAT FS",
                           "Failed to allocate cluster: no free clusters.");
    return 0;
  }

  // Mark the new cluster as EOF
  if (set_next_cluster(free_cluster, 0xFFFFFFFF) < 0) {
    fk::algorithms::kerror("FAT FS", "Failed to mark new cluster %u as EOF.",
                           free_cluster);
    return 0;
  }

  // Link the previous cluster to the new one, if a previous cluster exists
  if (previous_cluster != 0) {
    if (set_next_cluster(previous_cluster, free_cluster) < 0) {
      fk::algorithms::kerror(
          "FAT FS", "Failed to link previous cluster %u to new cluster %u.",
          previous_cluster, free_cluster);
      // Cleanup: Mark the newly allocated cluster as free again
      set_next_cluster(free_cluster, 0);
      return 0;
    }
  }

  // Clear the contents of the newly allocated cluster
  size_t cluster_total_size =
      m_boot_sector.sectors_per_cluster * m_boot_sector.bytes_per_sector;
  fk::memory::OwnPtr<uint8_t[]> cluster_buffer_own_ptr =
      fk::memory::adopt_own<uint8_t[]>(new uint8_t[cluster_total_size]);
  uint8_t *buffer = cluster_buffer_own_ptr.ptr();
  if (!buffer) {
    fk::algorithms::kerror(
        "FAT FS", "Failed to allocate buffer for clearing new cluster %u.",
        free_cluster);
    // Cleanup: Mark clusters as free again
    if (previous_cluster != 0)
      set_next_cluster(previous_cluster, 0xFFFFFFFF); // restore EOF
    set_next_cluster(free_cluster, 0);
    return 0;
  }
  memset(buffer, 0, cluster_total_size);
  if (write_cluster(free_cluster, buffer) < 0) {
    fk::algorithms::kerror("FAT FS", "Failed to clear new cluster %u.",
                           free_cluster);
    // Cleanup: Mark clusters as free again
    if (previous_cluster != 0)
      set_next_cluster(previous_cluster, 0xFFFFFFFF); // restore EOF
    set_next_cluster(free_cluster, 0);
    return 0;
  }

  fk::algorithms::klog("FAT FS", "Allocated new cluster %u (linked from %u).",
                       free_cluster, previous_cluster);
  return free_cluster;
}

void FatFileSystem::fat_name_to_string(const char filename[8],
                                       const char extension[3],
                                       fk::text::fixed_string<256> &out_name) {
  out_name.clear();
  size_t i = 0;
  for (i = 0; i < 8 && filename[i] != ' '; ++i) {
    out_name.push_back(filename[i]);
  }

  if (extension[0] != ' ') {
    out_name.push_back('.');
    for (i = 0; i < 3 && extension[i] != ' '; ++i) {
      out_name.push_back(extension[i]);
    }
  }
  out_name.push_back('\0'); // Ensure null termination
}

void FatFileSystem::string_to_fat_name(const char *in_name, char filename[8],
                                       char extension[3]) {
  memset(filename, ' ', 8);
  memset(extension, ' ', 3);

  const char *dot = strchr(in_name, '.');

  size_t name_len;
  if (dot) {
    name_len = dot - in_name;
  } else {
    name_len = strlen(in_name);
  }

  for (size_t i = 0; i < 8 && i < name_len; ++i) {
    filename[i] = (char)toupper(in_name[i]);
  }

  if (dot) {
    size_t ext_len = strlen(dot + 1);
    for (size_t i = 0; i < 3 && i < ext_len; ++i) {
      extension[i] = (char)toupper(dot[1 + i]);
    }
  }
}

// VNodeOps Implementations

static inline FatFileSystem *get_fat_fs(VNode *vnode) {
  if (!vnode || !vnode->fs_private) {
    fk::algorithms::kerror("FAT VNODE", "VNode or fs_private is null.");
    return nullptr;
  }
  return reinterpret_cast<FatFileSystem *>(vnode->fs_private);
}

static inline FatFilePrivateData *get_fat_file_private_data(VNode *vnode) {
  if (!vnode || !vnode->inode || !vnode->inode->data_block_pointers[0]) {
    fk::algorithms::kerror("FAT VNODE",
                           "VNode, inode, or private data is null.");
    return nullptr;
  }
  // In FatFileSystem, we store FatFilePrivateData in
  // inode->data_block_pointers[0] (a bit of a hack but avoids adding new fields
  // to VNode directly)
  return reinterpret_cast<FatFilePrivateData *>(
      vnode->inode->data_block_pointers[0]);
}

int FatFileSystem::fat_read(VNode *vnode, [[maybe_unused]] FileDescriptor *fd,
                            void *buffer, size_t size, size_t offset) {
  fk::algorithms::klog("FAT FS VNODE",
                       "fat_read for '%s', size %zu, offset %zu",
                       vnode->m_name.c_str(), size, offset);
  FatFileSystem *fat_fs = get_fat_fs(vnode);
  if (!fat_fs)
    return -1;
  FatFilePrivateData *file_data = get_fat_file_private_data(vnode);
  if (!file_data)
    return -1;

  if (offset >= file_data->size) {
    return 0; // EOF
  }

  size_t total_read_bytes = 0;
  uint32_t current_cluster = file_data->first_cluster;
  uint32_t cluster_size = fat_fs->m_boot_sector.sectors_per_cluster *
                          fat_fs->m_boot_sector.bytes_per_sector;

  // Find the starting cluster for the given offset
  uint32_t cluster_offset_in_file = offset / cluster_size;
  uint32_t byte_offset_in_cluster = offset % cluster_size;

  for (uint32_t i = 0; i < cluster_offset_in_file; ++i) {
    current_cluster = fat_fs->get_next_cluster(current_cluster);
    if (current_cluster == 0 || current_cluster == 0xFFFFFFFF) {
      fk::algorithms::kerror("FAT FS VNODE",
                             "Reached EOF or bad cluster while seeking.");
      return -1; // Should not happen if file_size is correct
    }
  }

  fk::memory::OwnPtr<uint8_t[]> cluster_buffer_own_ptr =
      fk::memory::adopt_own<uint8_t[]>(new uint8_t[cluster_size]);
  uint8_t *cluster_buffer = cluster_buffer_own_ptr.ptr();
  if (!cluster_buffer) {
    fk::algorithms::kerror("FAT FS VNODE",
                           "Failed to allocate cluster buffer for read.");
    return -1;
  }
  while (total_read_bytes < size && current_cluster != 0 &&
         current_cluster != 0xFFFFFFFF) {
    if (fat_fs->read_cluster(current_cluster, cluster_buffer) < 0) {
      fk::algorithms::kerror("FAT FS VNODE", "Failed to read cluster %u.",
                             current_cluster);
      return -1;
    }

    size_t bytes_to_read_in_cluster = cluster_size - byte_offset_in_cluster;
    size_t remaining_size = size - total_read_bytes;
    size_t copy_size = (bytes_to_read_in_cluster < remaining_size)
                           ? bytes_to_read_in_cluster
                           : remaining_size;
    copy_size = (copy_size < (file_data->size - offset - total_read_bytes))
                    ? copy_size
                    : (file_data->size - offset - total_read_bytes);

    if (copy_size == 0)
      break; // Reached end of relevant data

    memcpy(static_cast<uint8_t *>(buffer) + total_read_bytes,
           cluster_buffer + byte_offset_in_cluster, copy_size);
    total_read_bytes += copy_size;

    // For subsequent reads, start from the beginning of the next cluster
    byte_offset_in_cluster = 0;
    current_cluster = fat_fs->get_next_cluster(current_cluster);
  }

  return static_cast<int>(total_read_bytes);
}

int FatFileSystem::fat_write(VNode *vnode, [[maybe_unused]] FileDescriptor *fd,
                             const void *buffer, size_t size, size_t offset) {
  fk::algorithms::klog("FAT FS VNODE",
                       "fat_write for '%s', size %zu, offset %zu",
                       vnode->m_name.c_str(), size, offset);
  FatFileSystem *fat_fs = get_fat_fs(vnode);
  if (!fat_fs)
    return -1;
  FatFilePrivateData *file_data = get_fat_file_private_data(vnode);
  if (!file_data)
    return -1;

  size_t total_written_bytes = 0;
  uint32_t current_cluster = file_data->first_cluster;
  uint32_t cluster_size = fat_fs->m_boot_sector.sectors_per_cluster *
                          fat_fs->m_boot_sector.bytes_per_sector;

  // Find the starting cluster for the given offset, allocating if necessary
  uint32_t cluster_idx_to_reach = offset / cluster_size;
  uint32_t byte_offset_in_cluster = offset % cluster_size;

  uint32_t previous_cluster = 0; // For linking new clusters
  for (uint32_t i = 0; i <= cluster_idx_to_reach; ++i) {
    if (current_cluster == 0 || current_cluster == 0xFFFFFFFF) {
      // Need to allocate a new cluster
      uint32_t new_cluster = fat_fs->allocate_cluster(previous_cluster);
      if (new_cluster == 0) {
        fk::algorithms::kerror("FAT FS VNODE",
                               "Failed to allocate new cluster during write.");
        return -1;
      }
      if (previous_cluster == 0) { // First allocation for a new file
        file_data->first_cluster = new_cluster;
      }
      current_cluster = new_cluster;
    } else {
      previous_cluster = current_cluster;
      current_cluster = fat_fs->get_next_cluster(current_cluster);
    }
  }
  // After the loop, current_cluster points to the cluster we need to start
  // writing to And previous_cluster points to the cluster *before*
  // current_cluster (if it existed) or is 0 if current_cluster is the first
  // cluster. This logic is tricky. Let's re-think for simplicity: always follow
  // the chain until we need to write, then allocate.

  current_cluster = file_data->first_cluster;
  if (current_cluster == 0) { // Empty file, allocate first cluster
    current_cluster = fat_fs->allocate_cluster(0);
    if (current_cluster == 0)
      return -1;
    file_data->first_cluster = current_cluster;
    vnode->inode->data_block_pointers[0] = reinterpret_cast<uint64_t>(
        file_data); // Update VNode private data with new first cluster
  }

  previous_cluster = 0; // Track the cluster before the current one
  uint32_t current_file_offset = 0;

  while (current_file_offset + cluster_size <= offset) {
    previous_cluster = current_cluster;
    current_cluster = fat_fs->get_next_cluster(current_cluster);
    if (current_cluster == 0 || current_cluster == 0xFFFFFFFF) {
      // Need to allocate a new cluster to reach offset
      uint32_t new_cluster = fat_fs->allocate_cluster(previous_cluster);
      if (new_cluster == 0) {
        fk::algorithms::kerror(
            "FAT FS VNODE",
            "Failed to allocate new cluster to reach offset %zu.", offset);
        return -1;
      }
      current_cluster = new_cluster;
    }
    current_file_offset += cluster_size;
  }

  fk::memory::OwnPtr<uint8_t[]> cluster_buffer_own_ptr =
      fk::memory::adopt_own<uint8_t[]>(new uint8_t[cluster_size]);
  uint8_t *cluster_buffer = cluster_buffer_own_ptr.ptr();
  if (!cluster_buffer) {
    fk::algorithms::kerror("FAT FS VNODE",
                           "Failed to allocate cluster buffer for write.");
    return total_written_bytes > 0 ? static_cast<int>(total_written_bytes) : -1;
  }
  while (total_written_bytes < size) {
    if (current_cluster == 0 || current_cluster == 0xFFFFFFFF) {
      // Allocate new cluster if needed
      uint32_t new_cluster = fat_fs->allocate_cluster(previous_cluster);
      if (new_cluster == 0) {
        fk::algorithms::kerror("FAT FS VNODE",
                               "Failed to allocate new cluster during write.");
        return total_written_bytes > 0 ? static_cast<int>(total_written_bytes)
                                       : -1;
      }
      current_cluster = new_cluster;
    }

    // Read existing cluster data if we are not writing a full cluster and not
    // starting at offset 0 of the cluster
    if (byte_offset_in_cluster != 0 ||
        (size - total_written_bytes < cluster_size)) {
      if (fat_fs->read_cluster(current_cluster, cluster_buffer) < 0) {
        fk::algorithms::kerror("FAT FS VNODE",
                               "Failed to read cluster %u for partial write.",
                               current_cluster);
        return total_written_bytes > 0 ? static_cast<int>(total_written_bytes)
                                       : -1;
      }
    }

    size_t bytes_to_write_in_cluster = cluster_size - byte_offset_in_cluster;
    size_t remaining_size = size - total_written_bytes;
    size_t copy_size = (bytes_to_write_in_cluster < remaining_size)
                           ? bytes_to_write_in_cluster
                           : remaining_size;

    memcpy(cluster_buffer + byte_offset_in_cluster,
           static_cast<const uint8_t *>(buffer) + total_written_bytes,
           copy_size);

    if (fat_fs->write_cluster(current_cluster, cluster_buffer) < 0) {
      fk::algorithms::kerror("FAT FS VNODE", "Failed to write cluster %u.",
                             current_cluster);
      return total_written_bytes > 0 ? static_cast<int>(total_written_bytes)
                                     : -1;
    }
    total_written_bytes += copy_size;
    byte_offset_in_cluster =
        0; // Subsequent writes are from beginning of cluster

    previous_cluster = current_cluster;
    current_cluster = fat_fs->get_next_cluster(
        current_cluster); // Advance to next cluster in chain or get EOF/0
  }

  // Update file size if it grew
  if (offset + size > file_data->size) {
    file_data->size = offset + size;
    vnode->size = file_data->size;
    // TODO: Also update the directory entry in the parent directory
  }

  return static_cast<int>(total_written_bytes);
}

fk::memory::optional<fk::memory::OwnPtr<Filesystem>> FatFileSystem::probe(
    fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice> device,
    uint32_t first_sector) {
  fk::algorithms::klog("FAT FS", "Probing for FAT filesystem on LBA %u.",
                       first_sector);

  FatBootSector boot_sector;
  if (device->read_sectors(first_sector, 1, &boot_sector) < 0) {
    fk::algorithms::kerror("FAT FS", "Failed to read boot sector during probe.");
    return fk::memory::optional<fk::memory::OwnPtr<Filesystem>>();
  }

  fk::algorithms::kdebug("FAT FS", "--- Boot Sector Details (LBA %u) ---", first_sector);
  fk::algorithms::kdebug("FAT FS", "Signature: 0x%x", boot_sector.boot_sector_signature);
  fk::algorithms::kdebug("FAT FS", "Root Dir Entries: %u", boot_sector.root_dir_entries);
  fk::algorithms::kdebug("FAT FS", "Sectors per FAT32: %u", boot_sector.sectors_per_fat_32);
  fk::algorithms::kdebug("FAT FS", "FS Type (FAT32 field): '%.8s'", boot_sector.fs_type_32);
  fk::algorithms::kdebug("FAT FS", "FS Type (FAT16 field): '%.8s'", boot_sector.fs_type_16);
  fk::algorithms::kdebug("FAT FS", "Bytes per sector: %u", boot_sector.bytes_per_sector);
  fk::algorithms::kdebug("FAT FS", "FAT Count: %u", boot_sector.fat_count);
  fk::algorithms::kdebug("FAT FS", "-------------------------------------");

  // Basic FAT signature check
  if (boot_sector.boot_sector_signature != 0xAA55) {
    fk::algorithms::kdebug("FAT FS", "No FAT signature (0xAA55) found at LBA %u. Found: 0x%x", first_sector, boot_sector.boot_sector_signature);
    return fk::memory::optional<fk::memory::OwnPtr<Filesystem>>();
  }

  // Try to detect FAT32 first
  if (boot_sector.root_dir_entries == 0 &&
      boot_sector.sectors_per_fat_32 > 0 &&
      memcmp(boot_sector.fs_type_32, "FAT32   ", 8) == 0) {
    fk::algorithms::klog("FAT FS", "Detected FAT32 filesystem at LBA %u. Creating instance.", first_sector);
    return fk::memory::optional<fk::memory::OwnPtr<Filesystem>>(
        fk::memory::adopt_own<Filesystem>(
            new FatFileSystem(fk::types::move(device), first_sector)));
  }

  // If not FAT32, try to detect FAT12/16
  if (boot_sector.root_dir_entries >= 512 && boot_sector.root_dir_entries <= 5120) { // Assuming "Root entries <= 512-5120" means within this range
    // The FAT size check for FAT12/16 is implicit in determining the FAT type from total_clusters,
    // but a direct check for sectors_per_fat_16 can also be used.
    // Here we'll rely on the original determine_fat_type for the size aspect after a successful probe.
    if (memcmp(boot_sector.fs_type_16, "FAT12   ", 8) == 0) {
      fk::algorithms::klog("FAT FS", "Detected FAT12 filesystem at LBA %u. Creating instance.", first_sector);
      return fk::memory::optional<fk::memory::OwnPtr<Filesystem>>(
          fk::memory::adopt_own<Filesystem>(
              new FatFileSystem(fk::types::move(device), first_sector)));
    } else if (memcmp(boot_sector.fs_type_16, "FAT16   ", 8) == 0) {
      fk::algorithms::klog("FAT FS", "Detected FAT16 filesystem at LBA %u. Creating instance.", first_sector);
      return fk::memory::optional<fk::memory::OwnPtr<Filesystem>>(
          fk::memory::adopt_own<Filesystem>(
              new FatFileSystem(fk::types::move(device), first_sector)));
    }
  }

  // If neither, return empty optional
  fk::algorithms::kdebug("FAT FS", "No FAT filesystem detected at LBA %u based on heuristics.", first_sector);
  return fk::memory::optional<fk::memory::OwnPtr<Filesystem>>();
}

// Static object to register the FAT filesystem driver
struct FatFilesystemRegistrar {
  FatFilesystemRegistrar() {
    fk::algorithms::klog("FAT FS", "Registering FAT filesystem driver.");
    Filesystem::register_filesystem_driver(FatFileSystem::probe);
  }
};
static FatFilesystemRegistrar s_fat_registrar; // This global object's
                                               // constructor will be called
                                               // during static initialization

int FatFileSystem::fat_open(VNode *vnode, FileDescriptor *fd, int flags) {
  fk::algorithms::klog("FAT FS VNODE", "fat_open for '%s', flags 0x%x",
                       vnode->m_name.c_str(), flags);
  // No special actions needed for open in FAT, just return success.
  // Permissions and existence checks are handled by lookup/create.
  (void)fd;
  (void)flags;
  return 0;
}

int FatFileSystem::fat_close(VNode *vnode, FileDescriptor *fd) {
  fk::algorithms::klog("FAT FS VNODE", "fat_close for '%s'",
                       vnode->m_name.c_str());
  // No special actions needed for close in FAT.
  (void)fd;
  return 0;
}

int FatFileSystem::fat_lookup(VNode *vnode, [[maybe_unused]] FileDescriptor *fd,
                              const char *name,
                              fk::memory::RetainPtr<VNode> &out) {
  fk::algorithms::klog("FAT FS VNODE", "fat_lookup for '%s' in directory '%s'",
                       name, vnode->m_name.c_str());
  FatFileSystem *fat_fs = get_fat_fs(vnode);
  if (!fat_fs)
    return -1;
  if (vnode->type != VNodeType::Directory) {
    fk::algorithms::kwarn("FAT FS VNODE", "Lookup on non-directory vnode '%s'.",
                          vnode->m_name.c_str());
    return -1;
  }

  FatFilePrivateData *dir_data = get_fat_file_private_data(vnode);
  if (!dir_data) { // Root directory might not have private data directly
    if (vnode->inode_number != fat_fs->m_boot_sector.root_cluster &&
        fat_fs->m_fat_type == FatType::FAT32) {
      fk::algorithms::kerror("FAT FS VNODE",
                             "Directory vnode has no private data.");
      return -1;
    }
  }

  uint32_t current_cluster =
      (fat_fs->m_fat_type == FatType::FAT32 &&
       vnode->inode_number == fat_fs->m_boot_sector.root_cluster)
          ? fat_fs->m_boot_sector.root_cluster
          : (dir_data ? dir_data->first_cluster : 0);

  if (vnode->inode_number == 0 &&
      fat_fs->m_fat_type != FatType::FAT32) { // Root directory for FAT12/16
    current_cluster = 0; // Special case for root directory in FAT12/16, it's
                         // not a cluster chain
  }

  uint32_t cluster_size = fat_fs->m_boot_sector.sectors_per_cluster *
                          fat_fs->m_boot_sector.bytes_per_sector;
  fk::memory::OwnPtr<uint8_t[]> cluster_buffer_own_ptr =
      fk::memory::adopt_own<uint8_t[]>(new uint8_t[cluster_size]);
  uint8_t *cluster_buffer = cluster_buffer_own_ptr.ptr();
  if (!cluster_buffer) {
    fk::algorithms::kerror("FAT FS VNODE",
                           "Failed to allocate cluster buffer for lookup.");
    return -1;
  }

  // Loop through directory clusters (or root directory sectors for FAT12/16)
  bool found = false;
  uint32_t current_dir_sector =
      fat_fs->m_root_dir_start_sector; // For FAT12/16 root
  // Remove unused dir_entry_count

  do {
    if (current_cluster == 0) { // FAT12/16 Root Directory
      for (uint32_t sector_offset = 0;
           sector_offset < fat_fs->get_root_dir_sectors(); ++sector_offset) {
        if (fat_fs->m_block_device->read_sectors(
                current_dir_sector + sector_offset, 1, cluster_buffer) != 1) {
          fk::algorithms::kerror("FAT FS VNODE",
                                 "Failed to read root directory sector %u.",
                                 current_dir_sector + sector_offset);
          return -1;
        }

        for (uint32_t i = 0; i < cluster_size / sizeof(FatDirEntry); ++i) {
          FatDirEntry *entry = reinterpret_cast<FatDirEntry *>(
              cluster_buffer + (i * sizeof(FatDirEntry)));
          if ((uint8_t)entry->filename[0] == 0x00) {
            goto end_lookup; // End of directory
          }
          if ((uint8_t)entry->filename[0] == 0xE5) {
            continue; // Deleted entry
          }
          if ((entry->attributes & FatFileAttributes::LFN) ==
              FatFileAttributes::LFN) {
            // Handle LFN entries. For now, skip and focus on SFN.
            continue;
          }

          fk::text::fixed_string<256> entry_name;
          FatFileSystem::fat_name_to_string(entry->filename, entry->extension,
                                            entry_name);

          if (strcmp(entry_name.c_str(), name) == 0) {
            // Found the entry
            fk::algorithms::klog("FAT FS VNODE", "Found '%s' in directory.",
                                 name);
            found = true;

            auto new_vnode = fk::memory::adopt_retain(new VNode());
            new_vnode->m_name = entry_name.c_str();
            new_vnode->parent = vnode;
            new_vnode->ops = &FatFileSystem::s_fat_vnode_ops;
            new_vnode->fs_private =
                fat_fs; // Point back to the FatFileSystem instance

            uint32_t first_cluster = entry->first_cluster_low;
            if (fat_fs->m_fat_type == FatType::FAT32) {
              first_cluster |=
                  (static_cast<uint32_t>(entry->first_cluster_high) << 16);
            }
            new_vnode->inode_number =
                first_cluster; // Use first cluster as inode number for FAT
            new_vnode->size = entry->file_size;

            if (entry->attributes & FatFileAttributes::DIRECTORY) {
              new_vnode->type = VNodeType::Directory;
              // For directories, associate private data with the vnode
              FatFilePrivateData *new_dir_data = new FatFilePrivateData();
              new_dir_data->first_cluster = first_cluster;
              new_dir_data->current_offset = 0;
              new_dir_data->size =
                  entry->file_size; // Directory size is often 0 or not directly
                                    // used in FAT for actual content size
              new_vnode->inode =
                  new Inode(first_cluster); // Inode for directory
              new_vnode->inode->data_block_pointers[0] =
                  reinterpret_cast<uint64_t>(
                      new_dir_data); // Store private data here
            } else {
              new_vnode->type = VNodeType::Regular;
              // For regular files, associate private data with the vnode
              FatFilePrivateData *new_file_data = new FatFilePrivateData();
              new_file_data->first_cluster = first_cluster;
              new_file_data->current_offset = 0;
              new_file_data->size = entry->file_size;
              new_vnode->inode = new Inode(first_cluster); // Inode for file
              new_vnode->inode->data_block_pointers[0] =
                  reinterpret_cast<uint64_t>(
                      new_file_data); // Store private data here
            }
            out = new_vnode;
            break;
          }
        }
        if (found)
          break;
      }
    } else { // Cluster-chained directory (FAT32 root or any sub-directory)
      if (fat_fs->read_cluster(current_cluster, cluster_buffer) < 0) {
        fk::algorithms::kerror("FAT FS VNODE",
                               "Failed to read directory cluster %u.",
                               current_cluster);
        return -1;
      }

      for (uint32_t i = 0; i < cluster_size / sizeof(FatDirEntry); ++i) {
        FatDirEntry *entry = reinterpret_cast<FatDirEntry *>(
            cluster_buffer + (i * sizeof(FatDirEntry)));
        if ((uint8_t)entry->filename[0] == 0x00) {
          goto end_lookup; // End of directory
        }
        if ((uint8_t)entry->filename[0] == 0xE5) {
          continue; // Deleted entry
        }
        if ((entry->attributes & FatFileAttributes::LFN) ==
            FatFileAttributes::LFN) {
          // Handle LFN entries
          continue;
        }

        fk::text::fixed_string<256> entry_name;
        FatFileSystem::fat_name_to_string(entry->filename, entry->extension,
                                          entry_name);

        if (strcmp(entry_name.c_str(), name) == 0) {
          // Found the entry
          fk::algorithms::klog("FAT FS VNODE", "Found '%s' in directory.",
                               name);
          found = true;

          auto new_vnode = fk::memory::adopt_retain(new VNode());
          new_vnode->m_name = entry_name.c_str();
          new_vnode->parent = vnode;
          new_vnode->ops = &FatFileSystem::s_fat_vnode_ops;
          new_vnode->fs_private =
              fat_fs; // Point back to the FatFileSystem instance

          uint32_t first_cluster = entry->first_cluster_low;
          if (fat_fs->m_fat_type == FatType::FAT32) {
            first_cluster |=
                (static_cast<uint32_t>(entry->first_cluster_high) << 16);
          }
          new_vnode->inode_number = first_cluster;
          new_vnode->size = entry->file_size;

          if (entry->attributes & FatFileAttributes::DIRECTORY) {
            new_vnode->type = VNodeType::Directory;
            FatFilePrivateData *new_dir_data = new FatFilePrivateData();
            new_dir_data->first_cluster = first_cluster;
            new_dir_data->current_offset = 0;
            new_dir_data->size = entry->file_size;
            new_vnode->inode = new Inode(first_cluster);
            new_vnode->inode->data_block_pointers[0] =
                reinterpret_cast<uint64_t>(new_dir_data);
          } else {
            new_vnode->type = VNodeType::Regular;
            FatFilePrivateData *new_file_data = new FatFilePrivateData();
            new_file_data->first_cluster = first_cluster;
            new_file_data->current_offset = 0;
            new_file_data->size = entry->file_size;
            new_vnode->inode = new Inode(first_cluster);
            new_vnode->inode->data_block_pointers[0] =
                reinterpret_cast<uint64_t>(new_file_data);
          }
          out = new_vnode;
          break;
        }
      }
      if (found)
        break;

      current_cluster = fat_fs->get_next_cluster(current_cluster);
    }
  } while (current_cluster != 0 && current_cluster != 0xFFFFFFFF && !found);

end_lookup:
  if (!found) {
    fk::algorithms::kdebug("FAT FS VNODE", "'%s' not found in directory.",
                           name);
    return -1;
  }
  return 0;
}

int FatFileSystem::fat_create(VNode *vnode, [[maybe_unused]] FileDescriptor *fd,
                              const char *name, VNodeType type,
                              fk::memory::RetainPtr<VNode> &out) {
  fk::algorithms::klog("FAT FS VNODE",
                       "fat_create for '%s' in directory '%s', type %d", name,
                       vnode->m_name.c_str(), static_cast<int>(type));
  FatFileSystem *fat_fs = get_fat_fs(vnode);
  if (!fat_fs)
    return -1;
  if (vnode->type != VNodeType::Directory) {
    fk::algorithms::kwarn("FAT FS VNODE", "Create on non-directory vnode '%s'.",
                          vnode->m_name.c_str());
    return -1;
  }

  FatFilePrivateData *dir_data = get_fat_file_private_data(vnode);
  uint32_t current_cluster =
      (fat_fs->m_fat_type == FatType::FAT32 &&
       vnode->inode_number == fat_fs->m_boot_sector.root_cluster)
          ? fat_fs->m_boot_sector.root_cluster
          : (dir_data ? dir_data->first_cluster : 0);

  uint32_t cluster_size = fat_fs->m_boot_sector.sectors_per_cluster *
                          fat_fs->m_boot_sector.bytes_per_sector;
  fk::memory::OwnPtr<uint8_t[]> cluster_buffer_own_ptr =
      fk::memory::adopt_own<uint8_t[]>(new uint8_t[cluster_size]);
  uint8_t *cluster_buffer = cluster_buffer_own_ptr.ptr();
  if (!cluster_buffer) {
    fk::algorithms::kerror("FAT FS VNODE",
                           "Failed to allocate cluster buffer for create.");
    return -1;
  }

  uint32_t target_dir_sector = 0;  // For FAT12/16 root directory
  uint32_t target_dir_cluster = 0; // For FAT32 root or sub-directories
  uint32_t dir_entry_offset =
      0; // Offset within the cluster/sector where new entry will be written

  // Find a free directory entry slot
  bool found_free_slot = false;
  uint32_t search_cluster = current_cluster;
  uint32_t previous_search_cluster = 0;

  do {
    if (search_cluster == 0) { // FAT12/16 Root Directory
      for (uint32_t sector_offset = 0;
           sector_offset < fat_fs->get_root_dir_sectors(); ++sector_offset) {
        if (fat_fs->m_block_device->read_sectors(
                fat_fs->m_root_dir_start_sector + sector_offset, 1,
                cluster_buffer) != 1) {
          fk::algorithms::kerror(
              "FAT FS VNODE",
              "Failed to read root directory sector %u for create.",
              fat_fs->m_root_dir_start_sector + sector_offset);
          return -1;
        }
        for (uint32_t i = 0; i < cluster_size / sizeof(FatDirEntry); ++i) {
          FatDirEntry *entry = reinterpret_cast<FatDirEntry *>(
              cluster_buffer + (i * sizeof(FatDirEntry)));
          if ((uint8_t)entry->filename[0] == 0x00 ||
              (uint8_t)entry->filename[0] == 0xE5) { // Free or deleted slot
            found_free_slot = true;
            target_dir_sector = fat_fs->m_root_dir_start_sector + sector_offset;
            dir_entry_offset = i * sizeof(FatDirEntry);
            goto write_entry; // Exit nested loops
          }
        }
      }
      // If no free slot in root directory, we can't create (FAT12/16 root is
      // fixed size)
      fk::algorithms::kerror("FAT FS VNODE",
                             "FAT12/16 root directory is full.");
      return -1;
    } else { // Cluster-chained directory
      if (fat_fs->read_cluster(search_cluster, cluster_buffer) < 0) {
        fk::algorithms::kerror(
            "FAT FS VNODE", "Failed to read directory cluster %u for create.",
            search_cluster);
        return -1;
      }
      for (uint32_t i = 0; i < cluster_size / sizeof(FatDirEntry); ++i) {
        FatDirEntry *entry = reinterpret_cast<FatDirEntry *>(
            cluster_buffer + (i * sizeof(FatDirEntry)));
        if ((uint8_t)entry->filename[0] == 0x00 ||
            (uint8_t)entry->filename[0] == 0xE5) { // Free or deleted slot
          found_free_slot = true;
          target_dir_cluster = search_cluster;
          dir_entry_offset = i * sizeof(FatDirEntry);
          goto write_entry; // Exit nested loops
        }
      }
      previous_search_cluster = search_cluster;
      search_cluster = fat_fs->get_next_cluster(search_cluster);
      if (search_cluster == 0 ||
          search_cluster ==
              0xFFFFFFFF) { // End of cluster chain, allocate a new cluster
        uint32_t new_cluster =
            fat_fs->allocate_cluster(previous_search_cluster);
        if (new_cluster == 0) {
          fk::algorithms::kerror("FAT FS VNODE",
                                 "Failed to extend directory for new entry.");
          return -1;
        }
        target_dir_cluster = new_cluster;
        dir_entry_offset = 0; // Start at beginning of new cluster
        found_free_slot = true;
        goto write_entry;
      }
    }
  } while (!found_free_slot);

write_entry:
  if (!found_free_slot) {
    fk::algorithms::kerror(
        "FAT FS VNODE", "No free directory entry found to create '%s'.", name);
    return -1;
  }

  FatDirEntry *new_entry_ptr =
      reinterpret_cast<FatDirEntry *>(cluster_buffer + dir_entry_offset);
  memset(new_entry_ptr, 0, sizeof(FatDirEntry));
  FatFileSystem::string_to_fat_name(name, new_entry_ptr->filename,
                                    new_entry_ptr->extension);
  new_entry_ptr->attributes =
      (type == VNodeType::Directory) ? FatFileAttributes::DIRECTORY : 0;
  new_entry_ptr->file_size = 0;

  uint32_t new_first_cluster = fat_fs->allocate_cluster(
      0); // Allocate the first cluster for the new file/directory
  if (new_first_cluster == 0) {
    fk::algorithms::kerror(
        "FAT FS VNODE", "Failed to allocate first cluster for new entry '%s'.",
        name);
    return -1;
  }
  new_entry_ptr->first_cluster_low =
      static_cast<uint16_t>(new_first_cluster & 0xFFFF);
  if (fat_fs->m_fat_type == FatType::FAT32) {
    new_entry_ptr->first_cluster_high =
        static_cast<uint16_t>((new_first_cluster >> 16) & 0xFFFF);
  }

  // Write the updated directory sector/cluster back to disk
  if (current_cluster == 0) { // FAT12/16 Root Directory
    if (fat_fs->m_block_device->write_sectors(target_dir_sector, 1,
                                              cluster_buffer) != 1) {
      fk::algorithms::kerror(
          "FAT FS VNODE",
          "Failed to write root directory sector %u after create.",
          target_dir_sector);
      // TODO: Free the allocated cluster
      return -1;
    }
  } else { // Cluster-chained directory
    if (fat_fs->write_cluster(target_dir_cluster, cluster_buffer) < 0) {
      fk::algorithms::kerror(
          "FAT FS VNODE", "Failed to write directory cluster %u after create.",
          target_dir_cluster);
      // TODO: Free the allocated cluster
      return -1;
    }
  }

  // Create the new VNode
  auto new_vnode = fk::memory::adopt_retain(new VNode());
  new_vnode->m_name = name;
  new_vnode->type = type;
  new_vnode->parent = vnode;
  new_vnode->ops = &FatFileSystem::s_fat_vnode_ops;
  new_vnode->fs_private = fat_fs;
  new_vnode->inode_number = new_first_cluster;
  new_vnode->size = 0;

  FatFilePrivateData *new_private_data = new FatFilePrivateData();
  new_private_data->first_cluster = new_first_cluster;
  new_private_data->current_offset = 0;
  new_private_data->size = 0;
  new_vnode->inode = new Inode(new_first_cluster); // Inode for new entry
  new_vnode->inode->data_block_pointers[0] =
      reinterpret_cast<uint64_t>(new_private_data);

  out = new_vnode;
  vnode->dir_entries.push_back(
      DirEntry{name, new_vnode}); // Add to VNode's in-memory cache

  fk::algorithms::klog("FAT FS VNODE",
                       "Successfully created '%s' with first cluster %u.", name,
                       new_first_cluster);
  return 0;
}

int FatFileSystem::fat_readdir(VNode *vnode,
                               [[maybe_unused]] FileDescriptor *fd,
                               void *buffer, size_t max_entries) {
  fk::algorithms::klog("FAT FS VNODE", "fat_readdir for '%s'",
                       vnode->m_name.c_str());
  FatFileSystem *fat_fs = get_fat_fs(vnode);
  if (!fat_fs)
    return -1;
  if (vnode->type != VNodeType::Directory) {
    fk::algorithms::kwarn("FAT FS VNODE",
                          "Readdir on non-directory vnode '%s'.",
                          vnode->m_name.c_str());
    return -1;
  }

  DirEntry *output_buffer = reinterpret_cast<DirEntry *>(buffer);
  size_t entries_added = 0;

  FatFilePrivateData *dir_data = get_fat_file_private_data(vnode);
  uint32_t current_cluster =
      (fat_fs->m_fat_type == FatType::FAT32 &&
       vnode->inode_number == fat_fs->m_boot_sector.root_cluster)
          ? fat_fs->m_boot_sector.root_cluster
          : (dir_data ? dir_data->first_cluster : 0);

  uint32_t cluster_size = fat_fs->m_boot_sector.sectors_per_cluster *
                          fat_fs->m_boot_sector.bytes_per_sector;
  fk::memory::OwnPtr<uint8_t[]> cluster_buffer_own_ptr =
      fk::memory::adopt_own<uint8_t[]>(new uint8_t[cluster_size]);
  uint8_t *cluster_buffer = cluster_buffer_own_ptr.ptr();
  if (!cluster_buffer) {
    fk::algorithms::kerror("FAT FS VNODE",
                           "Failed to allocate cluster buffer for readdir.");
    return -1;
  }

  uint32_t current_dir_sector =
      fat_fs->m_root_dir_start_sector; // For FAT12/16 root

  do {
    if (current_cluster == 0) { // FAT12/16 Root Directory
      for (uint32_t sector_offset = 0;
           sector_offset < fat_fs->get_root_dir_sectors(); ++sector_offset) {
        if (fat_fs->m_block_device->read_sectors(
                current_dir_sector + sector_offset, 1, cluster_buffer) != 1) {
          fk::algorithms::kerror("FAT FS VNODE",
                                 "Failed to read root directory sector %u.",
                                 current_dir_sector + sector_offset);
          return -1;
        }

        for (uint32_t i = 0; i < cluster_size / sizeof(FatDirEntry); ++i) {
          FatDirEntry *entry = reinterpret_cast<FatDirEntry *>(
              cluster_buffer + (i * sizeof(FatDirEntry)));
          if ((uint8_t)entry->filename[0] == 0x00) {
            goto end_readdir; // End of directory
          }
          if ((uint8_t)entry->filename[0] == 0xE5 ||
              (entry->attributes & FatFileAttributes::LFN) ==
                  FatFileAttributes::LFN) {
            continue; // Deleted or LFN entry
          }

          if (entries_added >= max_entries)
            goto end_readdir;

          fk::text::fixed_string<256> entry_name;
          FatFileSystem::fat_name_to_string(entry->filename, entry->extension,
                                            entry_name);

          // For readdir, we typically return the name and type. We don't create
          // a full VNode yet. The DirEntry struct in FKernel's VFS expects a
          // VNode*. For now, we'll create a dummy VNode or fill a simplified
          // DirEntry if the API allows. Let's create minimal VNodes for
          // DirEntry for now, but this is inefficient if not cached.

          auto temp_vnode = fk::memory::adopt_retain(new VNode());
          temp_vnode->m_name = entry_name.c_str();
          temp_vnode->type = (entry->attributes & FatFileAttributes::DIRECTORY)
                                 ? VNodeType::Directory
                                 : VNodeType::Regular;
          temp_vnode->inode_number =
              entry->first_cluster_low; // Store first cluster as inode number
                                        // temporarily

          output_buffer[entries_added] =
              DirEntry{entry_name.c_str(), temp_vnode};
          entries_added++;
        }
      }
    } else {
      if (fat_fs->read_cluster(current_cluster, cluster_buffer) < 0) {
        fk::algorithms::kerror("FAT FS VNODE",
                               "Failed to read directory cluster %u.",
                               current_cluster);
        return -1;
      }

      for (uint32_t i = 0; i < cluster_size / sizeof(FatDirEntry); ++i) {
        FatDirEntry *entry = reinterpret_cast<FatDirEntry *>(
            cluster_buffer + (i * sizeof(FatDirEntry)));
        if ((uint8_t)entry->filename[0] == 0x00) {
          goto end_readdir; // End of directory
        }
        if ((uint8_t)entry->filename[0] == 0xE5 ||
            (entry->attributes & FatFileAttributes::LFN) ==
                FatFileAttributes::LFN) {
          continue; // Deleted or LFN entry
        }

        if (entries_added >= max_entries)
          goto end_readdir;

        fk::text::fixed_string<256> entry_name;
        FatFileSystem::fat_name_to_string(entry->filename, entry->extension,
                                          entry_name);

        auto temp_vnode = fk::memory::adopt_retain(new VNode());
        temp_vnode->m_name = entry_name.c_str();
        temp_vnode->type = (entry->attributes & FatFileAttributes::DIRECTORY)
                               ? VNodeType::Directory
                               : VNodeType::Regular;
        uint32_t first_cluster = entry->first_cluster_low;
        if (fat_fs->m_fat_type == FatType::FAT32) {
          first_cluster |=
              (static_cast<uint32_t>(entry->first_cluster_high) << 16);
        }
        temp_vnode->inode_number =
            first_cluster; // Store first cluster as inode number temporarily

        output_buffer[entries_added] = DirEntry{entry_name.c_str(), temp_vnode};
        entries_added++;
      }
      current_cluster = fat_fs->get_next_cluster(current_cluster);
    }
  } while (current_cluster != 0 && current_cluster != 0xFFFFFFFF);

end_readdir:
  return static_cast<int>(entries_added);
}

int FatFileSystem::fat_unlink(VNode *vnode, [[maybe_unused]] FileDescriptor *fd,
                              const char *name) {
  fk::algorithms::klog("FAT FS VNODE", "fat_unlink for '%s' in directory '%s'",
                       name, vnode->m_name.c_str());
  FatFileSystem *fat_fs = get_fat_fs(vnode);
  if (!fat_fs)
    return -1;
  if (vnode->type != VNodeType::Directory) {
    fk::algorithms::kwarn("FAT FS VNODE", "Unlink on non-directory vnode '%s'.",
                          vnode->m_name.c_str());
    return -1;
  }

  FatFilePrivateData *dir_data = get_fat_file_private_data(vnode);
  uint32_t current_cluster =
      (fat_fs->m_fat_type == FatType::FAT32 &&
       vnode->inode_number == fat_fs->m_boot_sector.root_cluster)
          ? fat_fs->m_boot_sector.root_cluster
          : (dir_data ? dir_data->first_cluster : 0);

  uint32_t cluster_size = fat_fs->m_boot_sector.sectors_per_cluster *
                          fat_fs->m_boot_sector.bytes_per_sector;
  fk::memory::OwnPtr<uint8_t[]> cluster_buffer_own_ptr =
      fk::memory::adopt_own<uint8_t[]>(new uint8_t[cluster_size]);
  uint8_t *cluster_buffer = cluster_buffer_own_ptr.ptr();
  if (!cluster_buffer) {
    fk::algorithms::kerror("FAT FS VNODE",
                           "Failed to allocate cluster buffer for unlink.");
    return -1;
  }

  bool found_entry = false;
  uint32_t target_dir_sector = 0;  // For FAT12/16 root
  uint32_t target_dir_cluster = 0; // For cluster-chained directories

  do {
    if (current_cluster == 0) { // FAT12/16 Root Directory
      for (uint32_t sector_offset = 0;
           sector_offset < fat_fs->get_root_dir_sectors(); ++sector_offset) {
        if (fat_fs->m_block_device->read_sectors(
                fat_fs->m_root_dir_start_sector + sector_offset, 1,
                cluster_buffer) != 1) {
          fk::algorithms::kerror(
              "FAT FS VNODE",
              "Failed to read root directory sector %u for unlink.",
              fat_fs->m_root_dir_start_sector + sector_offset);
          return -1;
        }

        for (uint32_t i = 0; i < cluster_size / sizeof(FatDirEntry); ++i) {
          FatDirEntry *entry = reinterpret_cast<FatDirEntry *>(
              cluster_buffer + (i * sizeof(FatDirEntry)));
          if ((uint8_t)entry->filename[0] == 0x00) {
            goto end_unlink; // End of directory
          }
          if ((uint8_t)entry->filename[0] == 0xE5 ||
              (entry->attributes & FatFileAttributes::LFN) ==
                  FatFileAttributes::LFN) {
            continue; // Deleted or LFN entry
          }

          fk::text::fixed_string<256> entry_name;
          FatFileSystem::fat_name_to_string(entry->filename, entry->extension,
                                            entry_name);

          if (strcmp(entry_name.c_str(), name) == 0) {
            found_entry = true;
            target_dir_sector = fat_fs->m_root_dir_start_sector + sector_offset;
            entry->filename[0] = 0xE5; // Mark as deleted
            goto write_back_dir_entry;
          }
        }
      }
    } else { // Cluster-chained directory
      if (fat_fs->read_cluster(current_cluster, cluster_buffer) < 0) {
        fk::algorithms::kerror(
            "FAT FS VNODE", "Failed to read directory cluster %u for unlink.",
            current_cluster);
        return -1;
      }

      for (uint32_t i = 0; i < cluster_size / sizeof(FatDirEntry); ++i) {
        FatDirEntry *entry = reinterpret_cast<FatDirEntry *>(
            cluster_buffer + (i * sizeof(FatDirEntry)));
        if ((uint8_t)entry->filename[0] == 0x00) {
          goto end_unlink; // End of directory
        }
        if ((uint8_t)entry->filename[0] == 0xE5 ||
            (entry->attributes & FatFileAttributes::LFN) ==
                FatFileAttributes::LFN) {
          continue; // Deleted or LFN entry
        }

        fk::text::fixed_string<256> entry_name;
        FatFileSystem::fat_name_to_string(entry->filename, entry->extension,
                                          entry_name);

        if (strcmp(entry_name.c_str(), name) == 0) {
          found_entry = true;
          target_dir_cluster = current_cluster;
          entry->filename[0] = 0xE5; // Mark as deleted
          goto write_back_dir_entry;
        }
      }
      current_cluster = fat_fs->get_next_cluster(current_cluster);
    }
  } while (current_cluster != 0 && current_cluster != 0xFFFFFFFF);

end_unlink:
  if (!found_entry) {
    fk::algorithms::kwarn("FAT FS VNODE", "Entry '%s' not found for unlink.",
                          name);
    return -1;
  }

write_back_dir_entry:
  if (target_dir_cluster == 0) { // FAT12/16 Root Directory
    if (fat_fs->m_block_device->write_sectors(target_dir_sector, 1,
                                              cluster_buffer) != 1) {
      fk::algorithms::kerror(
          "FAT FS VNODE",
          "Failed to write root directory sector %u after unlink.",
          target_dir_sector);
      return -1;
    }
  } else { // Cluster-chained directory
    if (fat_fs->write_cluster(target_dir_cluster, cluster_buffer) < 0) {
      fk::algorithms::kerror(
          "FAT FS VNODE", "Failed to write directory cluster %u after unlink.",
          target_dir_cluster);
      return -1;
    }
  }

  // TODO: Deallocate clusters associated with the unlinked file/directory

  // Remove from VNode's in-memory cache if present (inefficient, but for
  // static_vector)
  for (size_t i = 0; i < vnode->dir_entries.size(); ++i) {
    if (strcmp(vnode->dir_entries[i].m_name.c_str(), name) == 0) {
      vnode->dir_entries.erase(i);
      break;
    }
  }

  fk::algorithms::klog("FAT FS VNODE", "Successfully unlinked '%s'.", name);
  return 0;
}

} // namespace fkernel::fs::fat
