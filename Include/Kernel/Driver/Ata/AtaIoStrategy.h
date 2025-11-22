#pragma once

#include <Kernel/Driver/Ata/AtaDefs.h>
#include <LibFK/Types/types.h>

namespace fkernel::drivers::ata {

/**
 * @brief Encapsulates necessary I/O port information for an ATA device.
 * Adheres to Object Calisthenics rule 3: "Wrap All Primitives".
 */
class AtaDeviceIoInfo {
public:
  explicit AtaDeviceIoInfo(Bus bus, Drive drive, uint16_t base_io_port, uint16_t control_io_port)
    : m_bus(bus)
    , m_drive(drive)
    , m_base_io_port(base_io_port)
    , m_control_io_port(control_io_port) {}

  Bus bus() const { return m_bus; }
  Drive drive() const { return m_drive; }
  uint16_t base_io_port() const { return m_base_io_port; }
  uint16_t control_io_port() const { return m_control_io_port; }

private:
  Bus m_bus;
  Drive m_drive;
  uint16_t m_base_io_port;
  uint16_t m_control_io_port;
};

/**
 * @brief Abstract base class for ATA I/O strategies (PIO, DMA, UDMA, AHCI).
 * Adheres to Object Calisthenics rules by being small and focused.
 */
class AtaIoStrategy {
public:
  virtual ~AtaIoStrategy() = default;

  /**
   * @brief Reads sectors from the ATA device.
   *
   * @param device_io_info Information about the ATA device's I/O ports.
   * @param lba Starting Logical Block Address.
   * @param sector_count Number of sectors to read.
   * @param buffer Destination buffer.
   * @return Number of sectors read on success, negative error code on failure.
   */
  virtual int read_sectors(const AtaDeviceIoInfo& device_io_info, uint32_t lba, uint8_t sector_count, void* buffer) const = 0;

  /**
   * @brief Writes sectors to the ATA device.
   *
   * @param device_io_info Information about the ATA device's I/O ports.
   * @param lba Starting Logical Block Address.
   * @param sector_count Number of sectors to write.
   * @param buffer Source buffer.
   * @return Number of sectors written on success, negative error code on failure.
   */
  virtual int write_sectors(const AtaDeviceIoInfo& device_io_info, uint32_t lba, uint8_t sector_count, const void* buffer) = 0;
};

} // namespace fkernel::drivers::ata
