#pragma once

#include <Kernel/Block/BlockDevice.h>
#include <Kernel/Driver/Ata/AtaDefs.h>
#include <Kernel/Driver/Ata/AtaIoStrategy.h> // For fkernel::drivers::ata::AtaIoStrategy
#include <Kernel/Driver/Ata/AtaPioStrategy.h> // For fkernel::drivers::ata::AtaPioStrategy friend declaration
#include <LibFK/Container/static_vector.h>
#include <LibFK/Memory/retain_ptr.h>
#include <LibFK/Types/types.h>

/**
 * @brief ATA controller class
 */
class AtaController {
  friend class AtaCache; ///< Grants cache access to private I/O methods
  friend class fkernel::drivers::ata::AtaPioStrategy; ///< Grants PIO strategy access to raw I/O methods

private:
  AtaController() = default;

  /**
   * @brief Detect ATA devices on all buses
   */
  void detect_devices();

  /**
   * @brief Identify a device on a given bus/drive
   * @param bus Bus/channel
   * @param drive Drive (master/slave)
   * @param out Output structure for device info
   * @return true if the device exists and was identified
   */
  bool identify_device(Bus bus, Drive drive, AtaDeviceInfo &out);

  /**
   * @brief Get the base I/O port for a given bus
   */
  static uint16_t base_io(Bus bus);

  /**
   * @brief Get the control port for a given bus
   */
  static uint16_t ctrl_io(Bus bus);

  /**
   * @brief Read sectors via PIO
   * @param bus Bus/channel
   * @param drive Drive position
   * @param lba Starting logical block address
   * @param sector_count Number of sectors to read
   * @param buffer Destination buffer
   * @return Number of sectors read or negative error code
   */
  static int read_sectors_pio_impl(Bus bus, Drive drive, uint32_t lba, uint8_t sector_count,
                                   void *buffer);

  /**
   * @brief Write sectors via PIO
   * @param bus Bus/channel
   * @param drive Drive position
   * @param lba Starting logical block address
   * @param sector_count Number of sectors to write
   * @param buffer Source buffer
   * @return Number of sectors written or negative error code
   */
  static int write_sectors_pio_impl(Bus bus, Drive drive, uint32_t lba,
                                    uint8_t sector_count, const void *buffer);

  fk::containers::static_vector<fk::memory::RetainPtr<BlockDevice>, 16> m_devices;

public:
  /**
   * @brief Get singleton instance of the ATA controller
   */
  static AtaController &the();

  /**
   * @brief Initialize the ATA controller and detect devices
   */
  void initialize();
};
