#include <Kernel/Block/Partition/GptPartition.h>
#include <Kernel/Block/Partition/MbrPartition.h>
#include <Kernel/Block/Partition/PartitionParsingStrategy.h>
#include <Kernel/Block/PartitionManager.h>
#include <LibC/stdio.h> // For snprintf
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Memory/own_ptr.h>

PartitionManager::PartitionManager(fk::memory::RetainPtr<BlockDevice> device)
    : m_device(fk::types::move(device)) {
  fk::algorithms::klog("PARTITION MANAGER", "PartitionManager initialized.");
}

PartitionManager::~PartitionManager() = default;

void PartitionManager::set_strategy(
    fk::memory::OwnPtr<PartitionParsingStrategy> strategy) {
  m_strategy = fk::types::move(strategy);
  if (m_strategy) {
    fk::algorithms::klog("PARTITION MANAGER",
                         "Partition parsing strategy set.");
  } else {
    fk::algorithms::kwarn("PARTITION MANAGER",
                          "Partition parsing strategy set to null.");
  }
}

bool PartitionManager::is_gpt(const uint8_t *sector) const {
  if (sector[510] != 0x55 || sector[511] != 0xAA) {
    fk::algorithms::kdebug("PARTITION MANAGER",
                           "is_gpt: MBR signature 0x55AA not found.");
    return false;
  }

  const MbrEntry *entry = reinterpret_cast<const MbrEntry *>(sector + 446);

  if (static_cast<PartitionType>(entry->partition_type) ==
      PartitionType::GPT_PROTECTIVE_MBR) {
    fk::algorithms::kdebug("PARTITION MANAGER",
                           "is_gpt: Found GPT Protective MBR entry.");
    return true;
  }
  fk::algorithms::kdebug("PARTITION MANAGER",
                         "is_gpt: No GPT Protective MBR entry found.");
  return false;
}

bool PartitionManager::is_mbr(const uint8_t *sector) const {
  if (sector[510] != 0x55 || sector[511] != 0xAA) {
    fk::algorithms::kdebug("PARTITION MANAGER",
                           "is_mbr: MBR signature 0x55AA not found.");
    return false;
  }

  const MbrEntry *entry = reinterpret_cast<const MbrEntry *>(sector + 446);
  for (int i = 0; i < 4; ++i) {
    if (static_cast<PartitionType>(entry[i].partition_type) ==
        PartitionType::GPT_PROTECTIVE_MBR) {
      fk::algorithms::kdebug(
          "PARTITION MANAGER",
          "is_mbr: Found GPT Protective MBR entry. Not an MBR scheme.");
      return false;
    }
  }
  fk::algorithms::kdebug(
      "PARTITION MANAGER",
      "is_mbr: No GPT Protective MBR entry found. Likely an MBR scheme.");
  return true;
}

bool PartitionManager::read_sector(uint8_t *buffer, uint64_t lba) const {
  if (m_device->read(nullptr, nullptr, buffer, 512, lba * 512) != 512) {
    fk::algorithms::kwarn("PARTITION MANAGER", "Failed to read sector %llu.",
                          lba);
    return false;
  }
  return true;
}

PartitionManager::PartitionScheme
PartitionManager::detect_scheme(const uint8_t *sector0) const {
  alignas(16) uint8_t sector1[512];
  if (read_sector(sector1, 1)) {
    static const uint8_t gpt_sig[8] = {'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T'};
    if (memcmp(sector1, gpt_sig, 8) == 0) {
      fk::algorithms::klog("PARTITION MANAGER",
                           "GPT partition table detected (via signature).");
      return PartitionScheme::GPT;
    }
  }

  if (is_gpt(sector0)) {
    fk::algorithms::klog(
        "PARTITION MANAGER",
        "GPT partition table detected (protective MBR found).");
    return PartitionScheme::GPT;
  }

  if (is_mbr(sector0)) {
    fk::algorithms::klog("PARTITION MANAGER", "MBR partition table detected.");
    return PartitionScheme::MBR;
  }

  fk::algorithms::kwarn("PARTITION MANAGER", "Unknown partition table format.");
  return PartitionScheme::Unknown;
}

void PartitionManager::set_strategy_for_scheme(PartitionScheme scheme) {
  if (scheme == PartitionScheme::GPT) {
    m_strategy = fk::memory::adopt_own(new GptPartitionStrategy());
    fk::algorithms::klog("PARTITION MANAGER",
                         "Set strategy to GPTPartitionStrategy.");
    return;
  }
  if (scheme == PartitionScheme::MBR) {
    m_strategy = fk::memory::adopt_own(new MbrPartitionStrategy());
    fk::algorithms::klog("PARTITION MANAGER",
                         "Set strategy to MbrPartitionStrategy.");
    return;
  }
  fk::algorithms::kwarn("PARTITION MANAGER",
                        "Attempted to set strategy for unknown scheme.");
}

fk::memory::OwnPtr<uint8_t[]> PartitionManager::prepare_gpt_parsing_data(
    const uint8_t *sector1_header) const {
  fk::algorithms::klog("PARTITION MANAGER", "Preparing GPT parsing data.");
  const GptHeader *hdr = reinterpret_cast<const GptHeader *>(sector1_header);
  size_t pe_array_size =
      (size_t)hdr->num_partition_entries * hdr->partition_entry_size;

  if (pe_array_size == 0) {
    fk::algorithms::klog(
        "PARTITION MANAGER",
        "No partition entries found in header, pe_array_size is 0.");
    return nullptr;
  }
  fk::algorithms::klog("PARTITION MANAGER",
                       "GPT Partition Entry Array Size: %llu bytes "
                       "(num_entries: %u, entry_size: %u).",
                       pe_array_size, hdr->num_partition_entries,
                       hdr->partition_entry_size);

  size_t buffer_size = 512 + pe_array_size; // Header sector + partition entries
  auto gpt_buffer = fk::memory::adopt_own<uint8_t[]>(new uint8_t[buffer_size]);
  if (!gpt_buffer) {
    fk::algorithms::kerror(
        "PARTITION MANAGER",
        "Failed to allocate buffer for GPT parsing data (size: %llu).",
        buffer_size);
    return nullptr;
  }
  memcpy(gpt_buffer.ptr(), sector1_header, 512);
  fk::algorithms::klog("PARTITION MANAGER",
                       "Copied GPT header (sector 1) into buffer.");

  // Read the partition entry array that starts at hdr->partition_entries_lba
  // The current implementation of GptPartitionStrategy::parse assumes it
  // follows sector 1 (base + 512) for simplicity in the input buffer. However,
  // the spec says partition_entries_lba, which could be different. This needs
  // careful handling. For now, assuming hdr->partition_entries_lba is LBA 2 (so
  // it starts right after LBA 1 header).

  fk::algorithms::klog("PARTITION MANAGER",
                       "Attempting to read GPT partition entries from LBA "
                       "%llu, size %llu bytes.",
                       hdr->partition_entries_lba, pe_array_size);
  int bytes_read =
      m_device->read(nullptr, nullptr, gpt_buffer.ptr() + 512, pe_array_size,
                     hdr->partition_entries_lba * 512);

  if (bytes_read < 0 || (size_t)bytes_read != pe_array_size) {
    fk::algorithms::kwarn("PARTITION MANAGER",
                          "Failed to read GPT partition entries from LBA %llu. "
                          "Expected %llu bytes, read %d bytes.",
                          hdr->partition_entries_lba, pe_array_size,
                          bytes_read);
    return nullptr;
  }
  fk::algorithms::klog("PARTITION MANAGER",
                       "Successfully read %d bytes for GPT partition entries.",
                       bytes_read);

  return gpt_buffer;
}

PartitionDeviceList
PartitionManager::create_devices_from_entries(const PartitionEntry *entries,
                                              int count) {
  PartitionDeviceList partitions;
  for (int i = 0; i < count; ++i) {
    PartitionLocation location(entries[i].lba_start, entries[i].lba_count);
    partitions.add(fk::memory::adopt_retain(
        new PartitionBlockDevice(m_device, fk::types::move(location))));
  }
  return partitions;
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

  for (int i = 0; i < num_entries; ++i) {
    const PartitionEntry &entry = entries[i];
    char chs_info[64]; // Increased buffer size for safety
    if (entry.has_chs) {
      snprintf(chs_info, sizeof(chs_info), "CHS: %u/%u/%u - %u/%u/%u",
               entry.chs_start[0], entry.chs_start[1], entry.chs_start[2],
               entry.chs_end[0], entry.chs_end[1], entry.chs_end[2]);
    } else {
      snprintf(chs_info, sizeof(chs_info), "(LBA mode)");
    }
    const char *type_str = "Unknown";
    // A more comprehensive mapping of PartitionType to string would be ideal,
    // but for now, generic.
    switch (entry.type) {
    case PartitionType::FAT12:
      type_str = "FAT12";
      break;
    case PartitionType::FAT16:
      type_str = "FAT16";
      break;
    case PartitionType::Extended:
      type_str = "Extended";
      break;
    case PartitionType::FAT32:
      type_str = "FAT32";
      break;
    case PartitionType::LINUX_SWAP:
      type_str = "Linux Swap";
      break;
    case PartitionType::LINUX_FILESYSTEM:
      type_str = "Linux Filesystem";
      break;
    case PartitionType::GPT_PROTECTIVE_MBR:
      type_str = "GPT Protective MBR";
      break;
    case PartitionType::EFI_SYSTEM:
      type_str = "EFI System";
      break;
    default:
      type_str = "Unknown";
      break;
    }
    fk::algorithms::klog("PARTITION MANAGER",
                         "  Partition %d: Type 0x%x (%s), Bootable: %s, LBA "
                         "Start %u, Count %u sectors. %s",
                         i, static_cast<uint8_t>(entry.type), type_str,
                         entry.is_bootable ? "Yes" : "No", entry.lba_start,
                         entry.lba_count, chs_info);
  }

  fk::algorithms::klog("PARTITION MANAGER",
                       "Finished partition detection. %llu partitions found.",
                       detected_partitions.count());
  return detected_partitions;
}
