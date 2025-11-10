#pragma once

#include <Kernel/Driver/Ata/AtaDefs.h>
#include <LibFK/Types/types.h>

#pragma once

#include <Kernel/Driver/Ata/AtaDefs.h>
#include <LibFK/Types/types.h>

/**
 * @brief ATA controller class
 */
class AtaController {
  friend class AtaCache; ///< Grants cache access to private I/O methods

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
  uint16_t base_io(Bus bus);

  /**
   * @brief Get the control port for a given bus
   */
  uint16_t ctrl_io(Bus bus);

  /**
   * @brief Read sectors via PIO
   * @param bus Bus/channel
   * @param drive Drive position
   * @param lba Starting logical block address
   * @param sector_count Number of sectors to read
   * @param buffer Destination buffer
   * @return Number of sectors read or negative error code
   */
  int read_sectors_pio(Bus bus, Drive drive, uint32_t lba, uint8_t sector_count,
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
  int write_sectors_pio(Bus bus, Drive drive, uint32_t lba,
                        uint8_t sector_count, const void *buffer);

public:
  /**
   * @brief Get singleton instance of the ATA controller
   */
  static AtaController &the();

  /**
   * @brief Initialize the ATA controller and detect devices
   */
  void initialize();

  /**
   * @brief Read sectors via PIO using a device info struct
   */
  int read_sectors_pio(const AtaDeviceInfo &device, uint32_t lba, uint8_t count,
                       void *buffer);

  /**
   * @brief Write sectors via PIO using a device info struct
   */
  int write_sectors_pio(const AtaDeviceInfo &device, uint32_t lba,
                        uint8_t count, const void *buffer);
};
