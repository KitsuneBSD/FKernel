#pragma once

#include <Kernel/Driver/Storage/Partitions/partition.h>
#include <Kernel/Driver/Storage/Partitions/partition_list.h>
#include <LibFK/Container/vector.h>

class PartitionManager {
  PartitionList m_partitions;
  PartitionManager() = default;

public:
  static PartitionManager &the();

  void scan(fk::RefPtr<StorageDevice> device);
  void add_partition(fk::RefPtr<Partition> partition);

  bool has_partitions_for_device(fk::RefPtr<StorageDevice> device) const;

  const PartitionList &partitions() const { return m_partitions; }
};
