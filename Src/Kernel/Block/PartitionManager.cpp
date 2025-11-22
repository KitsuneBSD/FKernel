#include <Kernel/Block/Partition/GptPartition.h>
#include <Kernel/Block/Partition/MbrPartition.h>
#include <Kernel/Block/Partition/PartitionParsingStrategy.h>
#include <Kernel/Block/PartitionManager.h>
#include <LibC/stdio.h> // For snprintf
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Memory/own_ptr.h>

PartitionManager::PartitionManager(fk::memory::RetainPtr<BlockDevice> device)
    : m_device(fk::types::move(device)) {}

PartitionManager::~PartitionManager() = default;

void PartitionManager::set_strategy(
    fk::memory::OwnPtr<PartitionParsingStrategy> strategy) {
  m_strategy = fk::types::move(strategy);
}

bool PartitionManager::is_gpt(const uint8_t *sector) const {
  if (sector[510] != 0x55 || sector[511] != 0xAA)
    return false;

  const MbrEntry *entry = reinterpret_cast<const MbrEntry *>(sector + 446);

  return static_cast<PartitionType>(entry->partition_type) == PartitionType::GPT_PROTECTIVE_MBR;
}

bool PartitionManager::is_mbr(const uint8_t *sector) const {
  if (sector[510] != 0x55 || sector[511] != 0xAA)
    return false;

  const MbrEntry *entry = reinterpret_cast<const MbrEntry *>(sector + 446);
  for (int i = 0; i < 4; ++i) {
    if (static_cast<PartitionType>(entry[i].partition_type) == PartitionType::GPT_PROTECTIVE_MBR)
      return false;
  }

  return true;
}

bool PartitionManager::read_sector(uint8_t *buffer, uint64_t lba) const {
  if (m_device->read(nullptr, nullptr, buffer, 512, lba * 512) != 512) {
    fk::algorithms::kwarn("PARTITION MANAGER", "Failed to read sector %llu.", lba);
    return false;
  }
  return true;
}

PartitionManager::PartitionScheme PartitionManager::detect_scheme(const uint8_t* sector0) const {
    alignas(16) uint8_t sector1[512];
    if (read_sector(sector1, 1)) {
        static const uint8_t gpt_sig[8] = {'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T'};
        if (memcmp(sector1, gpt_sig, 8) == 0) {
            fk::algorithms::klog("PARTITION MANAGER", "GPT partition table detected (via signature).");
            return PartitionScheme::GPT;
        }
    }

    if (is_gpt(sector0)) {
        fk::algorithms::klog("PARTITION MANAGER", "GPT partition table detected (protective MBR found).");
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
        return;
    }
    if (scheme == PartitionScheme::MBR) {
        m_strategy = fk::memory::adopt_own(new MbrPartitionStrategy());
        return;
    }
}

fk::memory::OwnPtr<uint8_t[]> PartitionManager::prepare_gpt_parsing_data(const uint8_t* sector1_header) const {
    const GptHeader* hdr = reinterpret_cast<const GptHeader*>(sector1_header);
    size_t pe_array_size = (size_t)hdr->num_partition_entries * hdr->partition_entry_size;

    if (pe_array_size == 0) {
        fk::algorithms::klog("GPT", "No partition entries found in header.");
        return nullptr;
    }

    size_t buffer_size = 512 + pe_array_size;
    auto gpt_buffer = fk::memory::adopt_own<uint8_t[]>(new uint8_t[buffer_size]);
    memcpy(gpt_buffer.ptr(), sector1_header, 512);

    int bytes_read = m_device->read(nullptr, nullptr, gpt_buffer.ptr() + 512, pe_array_size, hdr->partition_entries_lba * 512);

    if (bytes_read < 0 || (size_t)bytes_read != pe_array_size) {
        fk::algorithms::kwarn("PARTITION MANAGER", "Failed to read GPT partition entries from LBA %llu.", hdr->partition_entries_lba);
        return nullptr;
    }

    return gpt_buffer;
}

PartitionDeviceList PartitionManager::create_devices_from_entries(const PartitionEntry* entries, int count) {
    PartitionDeviceList partitions;
    for (int i = 0; i < count; ++i) {
        PartitionLocation location(entries[i].lba_start, entries[i].lba_count);
        partitions.add(fk::memory::adopt_retain(new PartitionBlockDevice(m_device, fk::types::move(location))));
    }
    return partitions;
}

PartitionDeviceList PartitionManager::detect_partitions() {
    if (!m_device)
        return {};

    alignas(16) uint8_t sector0[512];
    if (!read_sector(sector0, 0))
        return {};

    PartitionScheme scheme = detect_scheme(sector0);
    set_strategy_for_scheme(scheme);

    if (!m_strategy)
        return {};

    const void* sector_for_parse = sector0;
    fk::memory::OwnPtr<uint8_t[]> gpt_buffer;

    if (scheme == PartitionScheme::GPT) {
        alignas(16) uint8_t sector1[512];
        if (!read_sector(sector1, 1))
            return {};
        gpt_buffer = prepare_gpt_parsing_data(sector1);
        if (!gpt_buffer)
            return {};
        sector_for_parse = gpt_buffer.ptr();
    }

    PartitionEntry entries[16];
    int num_entries = m_strategy->parse(sector_for_parse, entries, 16);

    if (num_entries <= 0)
        return {};

    PartitionDeviceList detected_partitions = create_devices_from_entries(entries, num_entries);

    for (int i = 0; i < num_entries; ++i) {
        const PartitionEntry& entry = entries[i];
        char chs_info[32];
        if (entry.has_chs) {
            snprintf(chs_info, sizeof(chs_info), "CHS: %u/%u/%u - %u/%u/%u",
                     entry.chs_start[0], entry.chs_start[1], entry.chs_start[2],
                     entry.chs_end[0], entry.chs_end[1], entry.chs_end[2]);
        } else {
            snprintf(chs_info, sizeof(chs_info), "");
        }
        fk::algorithms::klog("PARTITION MANAGER", "  Partition %d: Type 0x%x, LBA Start %u, Count %u sectors. %s",
                             i, static_cast<uint8_t>(entry.type), entry.lba_start, entry.lba_count, chs_info);
    }

    return detected_partitions;
}

