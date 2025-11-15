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

bool PartitionManager::is_mbr(const uint8_t *sector) const {
  return sector[510] == 0x55 && sector[511] == 0xAA;
}

bool PartitionManager::is_gpt(const uint8_t *sector) const {
  const char gpt_sig[8] = {'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T'};
  return memcmp(sector + 0x1FE - 510, gpt_sig, 8) == 0;
}

static_vector<RetainPtr<PartitionBlockDevice>, 16>
PartitionManager::detect_partitions() {
  static_vector<RetainPtr<PartitionBlockDevice>, 16> partitions;

  if (!m_device)
    return partitions;

  alignas(16) uint8_t sector0[512];
  if (m_device->read(nullptr, nullptr, sector0, 512, 0) != 512) {
    kwarn("PartitionManager", "Failed to read partition table from device.");
    return partitions;
  }

  if (is_mbr(sector0)) {
    m_strategy = adopt_own(new MbrPartitionStrategy());
  } else {
    // LBA 1 para GPT
    alignas(16) uint8_t sector1[512];
    if (m_device->read(nullptr, nullptr, sector1, 512, 1) != 512) {
      kwarn("PartitionManager", "Failed to read GPT header.");
      return partitions;
    }

    const char gpt_sig[8] = {'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T'};
    if (memcmp(sector1, gpt_sig, 8) == 0) {
      m_strategy = adopt_own(new GptPartitionStrategy());
    } else {
      kwarn("PartitionManager", "Unknown partition table type.");
      return partitions;
    }
  }

  PartitionEntry entries[16];
  int num_partitions = m_strategy->parse(sector0, entries, 16);

  for (int i = 0; i < num_partitions; ++i) {
    PartitionInfo info;
    info.device = m_device;
    info.lba_first = entries[i].lba_start;
    info.sectors_count = entries[i].lba_count;
    info.type = entries[i].type;

    auto part_dev = adopt_retain(new PartitionBlockDevice(move(info)));
    partitions.push_back(part_dev);
  }

  return partitions;
}
