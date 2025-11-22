#include <Kernel/Driver/Ata/AtaPioStrategy.h>
#include <Kernel/Driver/Ata/AtaController.h>

namespace fkernel::drivers::ata {

int AtaPioStrategy::read_sectors(const AtaDeviceIoInfo& device_io_info, uint32_t lba, uint8_t sector_count, void* buffer) const {
  return AtaController::read_sectors_pio_impl(device_io_info.bus(), device_io_info.drive(), lba, sector_count, buffer);
}

int AtaPioStrategy::write_sectors(const AtaDeviceIoInfo& device_io_info, uint32_t lba, uint8_t sector_count, const void* buffer) {
  return AtaController::write_sectors_pio_impl(device_io_info.bus(), device_io_info.drive(), lba, sector_count, buffer);
}

} // namespace fkernel::drivers::ata