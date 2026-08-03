#include <Kernel/Driver/Storage/Controllers/Ata/ata_device.h>
#include <LibFK/Algorithms/Logging/log.h>

fk::core::Result<size_t, fk::core::Error>
ATADevice::read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) {
  if (!m_strategy) {
    fk::algorithms::kerror("ATA", "[%s] Attempted read with null strategy", name().c_str());
    return fk::core::Error::InvalidParameter;
  }
  fk::algorithms::klog("ATA", "[%s] Read. LBA: %lu, count: %zu", name().c_str(),
                       start_sector, count);
  return m_strategy->read_sectors(start_sector, count, buffer);
}

fk::core::Result<size_t, fk::core::Error>
ATADevice::write_sectors(uint64_t start_sector, size_t count,
                         const uint8_t *buffer) {
  if (!m_strategy) {
    fk::algorithms::kerror("ATA", "[%s] Attempted write with null strategy", name().c_str());
    return fk::core::Error::InvalidParameter;
  }
  fk::algorithms::klog("ATA", "[%s] Write. LBA: %lu, count: %zu",
                        name().c_str(), start_sector, count);
  return m_strategy->write_sectors(start_sector, count, buffer);
}
