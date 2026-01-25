#include <Kernel/Driver/Storage/Partitions/partition.h>

Partition::Partition(fk::RefPtr<StorageDevice> parent,
                     uint64_t start, uint64_t count, fk::text::String name)
    : m_parent_device(parent), m_start_sector(start),
      m_partition_sector_count(count) {
  set_name(fk::types::move(name));
  // Validate parent device
  if (!parent) {
    fk::algorithms::kwarn("PARTITION", "Partition created with null parent device");
    return;
  }
  
  // Validate LBA range
  if (start + count > parent->sector_count().value()) {
    fk::algorithms::kwarn("PARTITION", "Partition LBA range exceeds parent device capacity");
    return;
  }
}

fk::core::Result<fk::RefPtr<Partition>, fk::core::Error>
Partition::create(fk::RefPtr<StorageDevice> parent, uint64_t start,
                  uint64_t count, fk::text::String name) {
  if (start + count > parent->sector_count().value())
    return fk::core::Error::InvalidParameter;

  return fk::make_ref<Partition>(parent, start, count, fk::types::move(name)).value();
}

fk::core::Result<size_t, fk::core::Error>
Partition::read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) {
  if (!is_within_bounds(start_sector, count))
    return fk::core::Error::InvalidParameter;

  return m_parent_device->read_sectors(m_start_sector + start_sector, count,
                                       buffer);
}

fk::core::Result<size_t, fk::core::Error>
Partition::write_sectors(uint64_t start_sector, size_t count,
                         const uint8_t *buffer) {
  if (!is_within_bounds(start_sector, count))
    return fk::core::Error::InvalidParameter;

  return m_parent_device->write_sectors(m_start_sector + start_sector, count,
                                        buffer);
}

bool Partition::is_within_bounds(uint64_t start, size_t count) const {
  return (start + count) <= m_partition_sector_count.value();
}
