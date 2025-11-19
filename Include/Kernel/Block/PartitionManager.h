#pragma once

#include <Kernel/Block/BlockDevice.h>
#include <Kernel/Block/PartitionDeviceList.h>
#include <Kernel/Block/PartitionEntry.h>

#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Memory/retain_ptr.h>

class PartitionParsingStrategy;

/**
 * @brief Manages partitions on a block device.
 *
 * This class uses a strategy pattern to detect partitions on a given block
 * device. It then acts as a factory to create PartitionDevice instances for
 * each detected partition.
 */
class PartitionManager {
public:
  /**
   * @brief Constructs a PartitionManager for a given block device.
   *
   * @param device The block device to manage.
   */
  explicit PartitionManager(fk::memory::RetainPtr<BlockDevice> device);
  ~PartitionManager();

  /**
   * @brief Sets the partition parsing strategy.
   *
   * @param strategy The strategy to use for parsing the partition table.
   */
  void set_strategy(fk::memory::OwnPtr<PartitionParsingStrategy> strategy);

  /**
   * @brief Detects partitions on the device using the current strategy.
   *
   * This method will parse the partition table of the block device and create
   * PartitionDevice instances for each valid partition found.
   *
   * @return A list of created PartitionDevice objects.
   *         Returns an empty list if no strategy is set or no partitions
   *         are found.
   */
  PartitionDeviceList detect_partitions();

private:
  enum class PartitionScheme {
      Unknown,
      MBR,
      GPT,
  };

  fk::memory::RetainPtr<BlockDevice> m_device;
  fk::memory::OwnPtr<PartitionParsingStrategy> m_strategy;

  bool is_mbr(const uint8_t *sector) const;
  bool is_gpt(const uint8_t *sector) const;
  
  bool read_sector(uint8_t* buffer, uint64_t lba) const;
  PartitionScheme detect_scheme(const uint8_t* sector0) const;
  void set_strategy_for_scheme(PartitionScheme scheme);
  fk::memory::OwnPtr<uint8_t[]> prepare_gpt_parsing_data(const uint8_t* sector1_header) const;
  PartitionDeviceList create_devices_from_entries(const PartitionEntry* entries, int count);
};
