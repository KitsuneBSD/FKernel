#include <Kernel/Block/Partition/GptPartition.h>
#include <Kernel/Block/Partition/MbrPartition.h>
#include <Kernel/Block/Partition/PartitionParsingStrategy.h>
#include <Kernel/Block/PartitionManager.h>
#include <Kernel/FileSystem/Fat/fat_fs.h> // Keep for FAT-specific includes if needed elsewhere, but logic moves
#include <Kernel/FileSystem/VirtualFS/filesystem.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <LibC/stdio.h> // For snprintf
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Memory/retain_ptr.h>

namespace fkernel::block {
class PartitionBlockDevice;
}

using fkernel::block::PartitionBlockDevice;

PartitionManager::PartitionManager(fk::memory::RetainPtr<BlockDevice> device)
    : m_device(device) {
  fk::algorithms::klog("PARTITION MANAGER", "PartitionManager created.");
}

PartitionManager::~PartitionManager() {
  fk::algorithms::klog("PARTITION MANAGER", "PartitionManager destroyed.");
}

void PartitionManager::mount_filesystem(
    fk::memory::OwnPtr<fkernel::fs::Filesystem> filesystem,
    int partition_index) {
  if (!filesystem) {
    fk::algorithms::kerror(
        "PARTITION MANAGER",
        "Cannot mount filesystem: filesystem object is null.");
    return;
  }

  int init_result = filesystem->initialize();
  if (init_result < 0) {
    fk::algorithms::kerror(
        "PARTITION MANAGER",
        "Failed to initialize filesystem for partition %d, error: %d.",
        partition_index, init_result);
    return;
  }

  auto root_vnode = filesystem->root_vnode();
  if (!root_vnode) {
    fk::algorithms::kerror(
        "PARTITION MANAGER",
        "Filesystem for partition %d returned a null root VNode.",
        partition_index);
    return;
  }

  char mount_name[16];
  const char *fs_type_str = "unknown";
  switch (filesystem->type()) {
  case fkernel::fs::Filesystem::Type::FAT:
    fs_type_str = "fat";
    break;
  case fkernel::fs::Filesystem::Type::RamFS:
    fs_type_str = "ramfs";
    break;
  case fkernel::fs::Filesystem::Type::DevFS:
    fs_type_str = "devfs";
    break;
  case fkernel::fs::Filesystem::Type::AtaRaw:
    fs_type_str = "ataraw";
    break;
  case fkernel::fs::Filesystem::Type::Unknown:
    // Fallthrough
    break;
  }
  snprintf(mount_name, sizeof(mount_name), "%s%d", fs_type_str,
           partition_index);

  // The root VNode from the filesystem already has its fs_private set to the
  // filesystem instance itself. We just need to update its name for the
  // mountpoint.
  root_vnode->m_name = mount_name;

  int mount_result = VirtualFS::the().mount(mount_name, root_vnode,
                                            fk::types::move(filesystem));
  if (mount_result < 0) {
    fk::algorithms::kerror("PARTITION MANAGER",
                           "Failed to mount %s filesystem at /%s.", fs_type_str,
                           mount_name);
    return;
  }
  // The filesystem is now owned by VirtualFS (via the Mountpoint's
  // m_fs_instance).

  fk::algorithms::klog("PARTITION MANAGER",
                       "Successfully mounted %s filesystem as /%s.",
                       fs_type_str, mount_name);
}

bool PartitionManager::read_sector(uint8_t *buffer, uint64_t sector) const {
  fk::algorithms::klog("PARTITION MANAGER",
                       "Read sector called for sector %llu.", sector);
  if (!m_device) {
    fk::algorithms::kerror("PARTITION MANAGER",
                           "Read sector: Block device is null.");
    return false;
  }
  // BlockDevice::read_sectors expects uint32_t lba and uint8_t sector_count.
  // Assuming 'sector' can fit in uint32_t and we read 1 sector.
  return m_device->read_sectors(static_cast<uint32_t>(sector), 1, buffer) >= 0;
}

PartitionManager::PartitionScheme
PartitionManager::detect_scheme(const uint8_t *sector0) const {
  (void)sector0; // Suppress unused parameter warning
  fk::algorithms::klog("PARTITION MANAGER", "Detect scheme called.");
  if (is_gpt(sector0)) {
    fk::algorithms::klog("PARTITION MANAGER", "Detected GPT scheme.");
    return PartitionScheme::GPT;
  }
  if (is_mbr(sector0)) {
    fk::algorithms::klog("PARTITION MANAGER", "Detected MBR scheme.");
    return PartitionScheme::MBR;
  }
  fk::algorithms::klog("PARTITION MANAGER",
                       "Detected Unknown partition scheme.");
  return PartitionScheme::Unknown;
}

bool PartitionManager::is_mbr(const uint8_t *sector) const {
  // MBR signature is 0xAA55 at offset 510
  return sector[510] == 0x55 && sector[511] == 0xAA;
}

bool PartitionManager::is_gpt(const uint8_t *sector) const {
  // GPT Protective MBR has partition type 0xEE at offset 450 (MBR entry 1, type
  // field) This is a heuristic, a full GPT check requires reading LBA1
  const MbrEntry *entry = reinterpret_cast<const MbrEntry *>(sector + 446);
  return entry[0].partition_type == 0xEE && is_mbr(sector);
}

void PartitionManager::set_strategy_for_scheme(PartitionScheme scheme) {
  fk::algorithms::klog("PARTITION MANAGER",
                       "Set strategy for scheme called for scheme %d.",
                       static_cast<int>(scheme));
  if (scheme == PartitionScheme::MBR) {
    m_strategy = fk::memory::adopt_own(new MbrPartitionStrategy());
  } else if (scheme == PartitionScheme::GPT) {
    m_strategy = fk::memory::adopt_own(new GptPartitionStrategy());
  } else {
    m_strategy = nullptr;
  }
}

fk::memory::OwnPtr<uint8_t[]> PartitionManager::prepare_gpt_parsing_data(
    const uint8_t *sector1_header) const {
  fk::algorithms::klog("PARTITION MANAGER", "prepare_gpt_parsing_data called.");
  const GptHeader *gpt_header =
      reinterpret_cast<const GptHeader *>(sector1_header);

  if (gpt_header->signature != 0x5452415020494645ULL) { // "EFI PART"
    fk::algorithms::kerror(
        "PARTITION MANAGER",
        "GPT header signature invalid in prepare_gpt_parsing_data.");
    return nullptr;
  }

  // Calculate size needed for header + all partition entries
  size_t partition_array_bytes = (size_t)gpt_header->num_partition_entries *
                                 gpt_header->partition_entry_size;
  size_t total_buffer_size = 512 + partition_array_bytes; // Header + entries

  fk::memory::OwnPtr<uint8_t[]> buffer =
      fk::memory::adopt_own<uint8_t[]>(new uint8_t[total_buffer_size]);
  if (!buffer) {
    fk::algorithms::kerror("PARTITION MANAGER",
                           "Failed to allocate buffer for GPT parsing data.");
    return nullptr;
  }

  // Copy LBA 1 (GPT Header) into the beginning of the buffer
  memcpy(buffer.ptr(), sector1_header, 512);

  // Read partition entry array (starts at LBA specified by
  // gpt_header->partition_entries_lba) Assuming partition_entries_lba is
  // typically 2, so the array starts right after the header. We need to read
  // from the underlying device at that LBA.
  uint64_t lba_start_of_entries = gpt_header->partition_entries_lba;
  size_t sectors_to_read =
      (partition_array_bytes + 511) / 512; // Round up to nearest sector

  for (size_t i = 0; i < sectors_to_read; ++i) {
    if (!read_sector(buffer.ptr() + 512 + (i * 512),
                     lba_start_of_entries + i)) {
      fk::algorithms::kerror("PARTITION MANAGER",
                             "Failed to read GPT partition entry sector %llu.",
                             lba_start_of_entries + i);
      return nullptr;
    }
  }

  fk::algorithms::klog("PARTITION MANAGER",
                       "Prepared GPT parsing data. Total size: %zu bytes.",
                       total_buffer_size);
  return buffer;
}

PartitionDeviceList
PartitionManager::create_devices_from_entries(const PartitionEntry *entries,
                                              int num_entries) {
  fk::algorithms::klog("PARTITION MANAGER",
                       "create_devices_from_entries called with %d entries.",
                       num_entries);
  PartitionDeviceList devices;
  for (int i = 0; i < num_entries; ++i) {
    const PartitionEntry &entry = entries[i];
    if (entry.lba_count == 0) {
      fk::algorithms::kdebug("PARTITION MANAGER",
                             "Skipping empty partition entry %d.", i);
      continue;
    }

    PartitionLocation location(entry.lba_start, entry.lba_count);
    fk::memory::RetainPtr<PartitionBlockDevice> partition_dev =
        fk::memory::adopt_retain(
            new PartitionBlockDevice(m_device, fk::types::move(location)));

    // Register partition device with DevFS.
    // The name should be based on the parent device name (e.g., ada0p1, ada0p2)
    // This part of registration is already handled in AtaController, but
    // conceptually, if PartitionManager were standalone, it would register
    // here. For now, just add to the list.
    devices.add(fk::types::move(partition_dev));
    fk::algorithms::klog("PARTITION MANAGER",
                         "Created PartitionBlockDevice for LBA %u, Count %u.",
                         entry.lba_start, entry.lba_count);
  }
  fk::algorithms::klog("PARTITION MANAGER",
                       "Finished creating %zu partition devices.",
                       devices.count());
  return devices;
}

PartitionDeviceList PartitionManager::detect_partitions() {
  fk::algorithms::klog("PARTITION MANAGER", "Starting partition detection.");
  if (!m_device) {
    fk::algorithms::kerror("PARTITION MANAGER",
                           "Cannot detect partitions: Block device is null.");
    return {};
  }

  alignas(16) uint8_t sector0[512];
  if (!read_sector(sector0, 0)) {
    fk::algorithms::kerror(
        "PARTITION MANAGER",
        "Failed to read sector 0 for partition scheme detection.");
    return {};
  }

  PartitionScheme scheme = detect_scheme(sector0);
  set_strategy_for_scheme(scheme);

  if (!m_strategy) {
    fk::algorithms::kwarn("PARTITION MANAGER",
                          "No partition parsing strategy found for scheme %d.",
                          static_cast<int>(scheme));
    return {};
  }

  const void *sector_for_parse = sector0;
  fk::memory::OwnPtr<uint8_t[]> gpt_buffer;

  if (scheme == PartitionScheme::GPT) {
    alignas(16) uint8_t sector1[512];
    if (!read_sector(sector1, 1)) {
      fk::algorithms::kerror("PARTITION MANAGER",
                             "Failed to read sector 1 for GPT header.");
      return {};
    }
    gpt_buffer = prepare_gpt_parsing_data(sector1);
    if (!gpt_buffer) {
      fk::algorithms::kerror("PARTITION MANAGER",
                             "Failed to prepare GPT parsing data.");
      return {};
    }
    sector_for_parse = gpt_buffer.ptr();
  }

  PartitionEntry
      entries[16]; // Max 16 partitions, common for MBR/GPT primary entries
  int num_entries = m_strategy->parse(sector_for_parse, entries, 16);

  if (num_entries <= 0) {
    fk::algorithms::klog("PARTITION MANAGER",
                         "No partitions detected by the selected strategy.");
    return {};
  }
  fk::algorithms::klog("PARTITION MANAGER",
                       "Detected %d partition(s) using the current strategy.",
                       num_entries);

  PartitionDeviceList detected_partitions =
      create_devices_from_entries(entries, num_entries);

  for (size_t i = 0; i < detected_partitions.count(); ++i) {
    fk::memory::OwnPtr<fkernel::fs::Filesystem> filesystem_to_mount = nullptr;
    fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice>
        partition_device = detected_partitions.begin()[i];

    // Iterate through registered filesystem drivers to probe the partition
    for (auto probe_func : fkernel::fs::Filesystem::s_filesystem_drivers) {
      if (probe_func) {
        fk::algorithms::klog("PARTITION MANAGER",
                             "Attempting to probe partition %zu with a "
                             "registered filesystem driver.",
                             i);
        fk::memory::optional<fk::memory::OwnPtr<fkernel::fs::Filesystem>>
            probed_fs = probe_func(
                partition_device,
                0); // Pass 0 as first_sector for now, adjust if needed
        if (probed_fs.has_value()) {
          filesystem_to_mount = fk::types::move(probed_fs.value());
          break; // Found and created a filesystem, stop probing with other
                 // drivers
        }
      }
    }

    if (filesystem_to_mount) {
      mount_filesystem(fk::types::move(filesystem_to_mount), i);
    } else {
      fk::algorithms::kwarn(
          "PARTITION MANAGER",
          "No supported filesystem detected for partition %zu.", i);
    }
  }

  fk::algorithms::klog("PARTITION MANAGER",
                       "Finished partition detection. %llu partitions found.",
                       detected_partitions.count());
  return detected_partitions;
}
