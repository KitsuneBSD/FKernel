#pragma once

#include <Kernel/Driver/Storage/Partitions/partition.h>
#include <Kernel/Driver/Storage/Partitions/partition_list.h>
#include <LibFK/Container/vector.h>

namespace fkernel {

class PartitionManager {
  PartitionList m_partitions;
  bool m_is_initialized{false};

  PartitionManager() = default;
  PartitionManager(const PartitionManager &) = delete;
  PartitionManager &operator=(const PartitionManager &) = delete;

public:
  static PartitionManager &the();

  bool is_initialized() const { return m_is_initialized; }
  void initialize();

  void scan(fk::RefPtr<StorageDevice> device);
  void add_partition(fk::RefPtr<Partition> partition);

  bool has_partitions_for_device(fk::RefPtr<StorageDevice> device) const;

  const PartitionList &partitions() const { return m_partitions; }
};

} // namespace fkernel

using fkernel::PartitionManager;
