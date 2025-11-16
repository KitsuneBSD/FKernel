#include "Kernel/Block/Partition/GptPartition.h"
#include "Kernel/Block/Partition/MbrPartition.h"
#include "LibFK/Memory/own_ptr.h"
#include <Kernel/Block/Partition/PartitionParsingStrategy.h>
#include <Kernel/Block/PartitionManager.h>
#include <LibFK/Algorithms/log.h>
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
  return sector[510] == 0x55 && sector[511] == 0xAA;
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
  bool has_gpt = false;

  if (m_device->read(nullptr, nullptr, sector1, 512, 1) == 512) {
    static const uint8_t gpt_sig[8] = {'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T'};
    has_gpt = memcmp(sector1, gpt_sig, 8) == 0;
  }

  const void *sector_for_parse = nullptr;

  if (has_gpt) {
    klog("PARTITION MANAGER", "GPT partition table detected.");
    m_strategy = adopt_own(new GptPartitionStrategy());

    sector_for_parse = sector1;
  } else { // GPT signature not found in sector 1
    bool sector0_has_mbr_sig = is_mbr(sector0);

    if (!sector0_has_mbr_sig) {
      // Neither GPT signature in sector 1 nor MBR signature in sector 0.
      kwarn("PARTITION MANAGER", "Unknown partition table format (no GPT sig "
                                 "in sector 1, no MBR sig in sector 0).");
      return partitions;
    }

    // Sector 0 has MBR signature. Now, check if it's a GPT protective MBR.
    if (is_gpt(sector0)) {
      // Case 2a: Sector 0 has MBR signature AND is_gpt(sector0) is true.
      // This means it's a GPT disk with a protective MBR.
      klog("PARTITION MANAGER",
           "GPT partition table detected (via protective MBR in sector 0).");
      m_strategy = adopt_own(new GptPartitionStrategy());
      sector_for_parse = sector1; // GPT header is in sector 1
    } else {
      // Case 2b: Sector 0 has MBR signature but is NOT a GPT protective MBR.
      // This is a standard MBR disk.
      klog("PARTITION MANAGER", "MBR partition table detected.");
      m_strategy = adopt_own(new MbrPartitionStrategy());
      sector_for_parse = sector0; // MBR entries are in sector 0
    }
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
