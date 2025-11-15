#pragma once

#include <Kernel/Block/BlockDevice.h>
#include <Kernel/Block/PartitionDevice.h>
#include <Kernel/Block/PartitionEntry.h>

#include <LibFK/Container/static_vector.h>
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
  explicit PartitionManager(RetainPtr<BlockDevice> dev) : m_device(move(dev)) {}
  ~PartitionManager();

  /**
   * @brief Sets the partition parsing strategy.
   *
   * @param strategy The strategy to use for parsing the partition table.
   */
  void set_strategy(OwnPtr<PartitionParsingStrategy> strategy);

  /**
   * @brief Detects partitions on the device using the current strategy.
   *
   * This method will parse the partition table of the block device and create
   * PartitionDevice instances for each valid partition found.
   *
   * @return A vector of RetainPtrs to the created PartitionDevice objects.
   *         Returns an empty vector if no strategy is set or no partitions
   *         are found.
   */
  static_vector<RetainPtr<PartitionBlockDevice>, 16>
  detect_partitions(); // Changed from StaticVector and PartitionDevice

private:
  RetainPtr<BlockDevice> m_device;
  OwnPtr<PartitionParsingStrategy> m_strategy;

  bool is_mbr(const uint8_t *sector) const;
  bool is_gpt(const uint8_t *sector) const;
};
