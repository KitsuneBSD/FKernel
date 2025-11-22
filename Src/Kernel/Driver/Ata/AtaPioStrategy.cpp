#include <Kernel/Driver/Ata/AtaController.h>
#include <Kernel/Driver/Ata/AtaPioStrategy.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel::drivers::ata {

int AtaPioStrategy::read_sectors(const AtaDeviceIoInfo &device_io_info,
                                 uint32_t lba, uint8_t sector_count,
                                 void *buffer) const {
  fk::algorithms::klog(
      "ATA PIO STRATEGY",
      "Read sectors request for LBA %u, count %u on Bus: %d, Drive: %d.", lba,
      sector_count, static_cast<int>(device_io_info.bus()),
      static_cast<int>(device_io_info.drive()));
  int result = AtaController::read_sectors_pio_impl(
      device_io_info.bus(), device_io_info.drive(), lba, sector_count, buffer);
  if (result < 0) {
    fk::algorithms::kerror(
        "ATA PIO STRATEGY",
        "Read sectors failed for LBA %u, count %u. Result: %d", lba,
        sector_count, result);
  } else {
    fk::algorithms::klog(
        "ATA PIO STRATEGY",
        "Read sectors completed for LBA %u, count %u. Result: %d", lba,
        sector_count, result);
  }
  return result;
}

int AtaPioStrategy::write_sectors(const AtaDeviceIoInfo &device_io_info,
                                  uint32_t lba, uint8_t sector_count,
                                  const void *buffer) {
  fk::algorithms::klog(
      "ATA PIO STRATEGY",
      "Write sectors request for LBA %u, count %u on Bus: %d, Drive: %d.", lba,
      sector_count, static_cast<int>(device_io_info.bus()),
      static_cast<int>(device_io_info.drive()));
  int result = AtaController::write_sectors_pio_impl(
      device_io_info.bus(), device_io_info.drive(), lba, sector_count, buffer);
  if (result < 0) {
    fk::algorithms::kerror(
        "ATA PIO STRATEGY",
        "Write sectors failed for LBA %u, count %u. Result: %d", lba,
        sector_count, result);
  } else {
    fk::algorithms::klog(
        "ATA PIO STRATEGY",
        "Write sectors completed for LBA %u, count %u. Result: %d", lba,
        sector_count, result);
  }
  return result;
}

} // namespace fkernel::drivers::ata
