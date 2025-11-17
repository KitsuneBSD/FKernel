#include <Kernel/Block/Partition/GptPartition.h>
#include <Kernel/Block/Partition/MbrPartition.h>
#include <Kernel/Block/Partition/PartitionParsingStrategy.h>
#include <Kernel/Block/PartitionManager.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/own_ptr.h>
#include <LibFK/new.h>

PartitionManager::~PartitionManager() = default;

void PartitionManager::set_strategy(OwnPtr<PartitionParsingStrategy> strategy) {
  m_strategy = move(strategy);
}

bool PartitionManager::is_gpt(const uint8_t *sector) const {
  if (sector[510] != 0x55 || sector[511] != 0xAA)
    return false;

  const MbrEntry *entry = reinterpret_cast<const MbrEntry *>(sector + 446);

  return entry->partition_type == 0xEE;
}

bool PartitionManager::is_mbr(const uint8_t *sector) const {
  if (sector[510] != 0x55 || sector[511] != 0xAA)
    return false;

  const MbrEntry *entry = reinterpret_cast<const MbrEntry *>(sector + 446);
  for (int i = 0; i < 4; ++i) {
    if (entry[i].partition_type == 0xEE)
      return false;
  }

  return true;
}

static_vector<RetainPtr<PartitionBlockDevice>, 16>
PartitionManager::detect_partitions() {
  static_vector<RetainPtr<PartitionBlockDevice>, 16> partitions;

  if (!m_device)
    return partitions;

  alignas(16) uint8_t sector0[512];
  if (m_device->read(nullptr, nullptr, sector0, 512, 0) != 512) {
    kwarn("PARTITION MANAGER", "Failed to read sector 0.");
    return partitions;
  }

  alignas(16) uint8_t sector1[512];
  bool has_gpt_header = false;
  if (m_device->read(nullptr, nullptr, sector1, 512, 512) == 512) {
    static const uint8_t gpt_sig[8] = {'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T'};
    if (memcmp(sector1, gpt_sig, 8) == 0) {
      has_gpt_header = true;
    }
  }

  const void *sector_for_parse = nullptr;
  OwnPtr<uint8_t[]> gpt_buffer;

  if (has_gpt_header) {
    if (is_gpt(sector0)) {
      klog("PARTITION MANAGER",
           "GPT partition table detected (protective MBR found).");
    } else {
      klog("PARTITION MANAGER",
           "GPT partition table detected (via signature).");
    }

    m_strategy = adopt_own(new GptPartitionStrategy());
    const GptHeader *hdr = reinterpret_cast<const GptHeader *>(sector1);

    size_t pe_array_size =
        (size_t)hdr->num_partition_entries * hdr->partition_entry_size;
    if (pe_array_size == 0) {
      klog("GPT", "No partition entries found in header.");
      return partitions;
    }

    size_t buffer_size = 512 + pe_array_size;
    gpt_buffer = adopt_own<uint8_t[]>(new uint8_t[buffer_size]);

    memcpy(gpt_buffer.ptr(), sector1, 512);

    int bytes_read = m_device->read(nullptr, nullptr, gpt_buffer.ptr() + 512, pe_array_size,
                                      hdr->partition_entries_lba * 512);
    if (bytes_read < 0 || (size_t)bytes_read != pe_array_size) {
      kwarn("PARTITION MANAGER",
            "Failed to read GPT partition entries from LBA %llu.",
            hdr->partition_entries_lba);
      return partitions;
    }

    sector_for_parse = gpt_buffer.ptr();

  } else if (is_mbr(sector0)) {
    klog("PARTITION MANAGER", "MBR partition table detected.");
    m_strategy = adopt_own(new MbrPartitionStrategy());
    sector_for_parse = sector0;
  } else {
    kwarn("PARTITION MANAGER", "Unknown partition table format.");
    return partitions;
  }

  if (!m_strategy) {
    return partitions;
  }

  PartitionEntry entries[16];
  int num = m_strategy->parse(sector_for_parse, entries, 16);

  if (num <= 0)
    return partitions;

  for (int i = 0; i < num; ++i) {
    PartitionInfo info;
    info.device = m_device;
    info.lba_first = entries[i].lba_start;
    info.sectors_count = entries[i].lba_count;
    info.type = entries[i].type;

    partitions.push_back(adopt_retain(new PartitionBlockDevice(move(info))));
  }

  return partitions;
}
