#pragma once

#include <Kernel/Driver/Ata/AtaIoStrategy.h>

namespace fkernel::drivers::ata {

/**
 * @brief PIO (Programmed I/O) implementation of AtaIoStrategy.
 * Handles read and write operations using PIO mode.
 */
class AtaPioStrategy : public AtaIoStrategy {
public:
  /**
   * @brief Reads sectors from the ATA device using PIO.
   *
   * @param device_io_info Information about the ATA device's I/O ports.
   * @param lba Starting Logical Block Address.
   * @param sector_count Number of sectors to read.
   * @param buffer Destination buffer.
   * @return Number of sectors read on success, negative error code on failure.
   */
  int read_sectors(const AtaDeviceIoInfo& device_io_info, uint32_t lba, uint8_t sector_count, void* buffer) const override;

  /**
   * @brief Writes sectors to the ATA device using PIO.
   *
   * @param device_io_info Information about the ATA device's I/O ports.
   * @param lba Starting Logical Block Address.
   * @param sector_count Number of sectors to write.
   * @param buffer Source buffer.
   * @return Number of sectors written on success, negative error code on failure.
   */
  int write_sectors(const AtaDeviceIoInfo& device_io_info, uint32_t lba, uint8_t sector_count, const void* buffer) override;
};

} // namespace fkernel::drivers::ata
