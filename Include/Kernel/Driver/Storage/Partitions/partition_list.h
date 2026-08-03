#pragma once

#include <Kernel/Driver/Storage/Partitions/partition.h>
#include <LibFK/Container/Sequence/vector.h>

class PartitionList {
  fk::containers::Vector<fk::RefPtr<Partition>> m_partitions;

public:
  void add(fk::RefPtr<Partition> partition) {
    m_partitions.push_back(partition);
  }
  const fk::containers::Vector<fk::RefPtr<Partition>> &all() const {
    return m_partitions;
  }
};
