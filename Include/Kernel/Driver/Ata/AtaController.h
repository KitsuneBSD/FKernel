#pragma once

#include <Kernel/Driver/Ata/AtaBlockDevice.h>
#include <LibFK/Types/types.h>

/**
 * @brief Base I/O addresses for ATA primary and secondary channels
 */
#define ATA_PRIMARY_BASE 0x1F0
#define ATA_SECONDARY_BASE 0x170
#define ATA_PRIMARY_CTRL 0x3F6
#define ATA_SECONDARY_CTRL 0x376

/**
 * @brief ATA register offsets
 */
#define ATA_REG_DATA 0x00
#define ATA_REG_STATUS 0x07
#define ATA_REG_COMMAND 0x07
#define ATA_REG_SECCOUNT 0x02
#define ATA_REG_LBA0 0x03
#define ATA_REG_LBA1 0x04
#define ATA_REG_LBA2 0x05
#define ATA_REG_HDDEVSEL 0x06

/**
 * @brief ATA commands and status flags
 */
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_BUSY 0x80

/**
 * @brief ATA bus/channel
 */
enum class Bus {
  Primary,  ///< Primary channel
  Secondary ///< Secondary channel
};

/**
 * @brief Drive position on the bus
 */
enum class Drive {
  Master, ///< Master drive
  Slave   ///< Slave drive
};

/**
 * @brief Information about an ATA device
 */
struct AtaDeviceInfo {
  Bus bus;        ///< ATA bus/channel
  Drive drive;    ///< Drive position on the bus
  bool exists;    ///< True if the device exists
  char model[41]; ///< Device model string (null-terminated)
};

/**
 * @brief ATA controller for managing devices and performing PIO I/O
 *
 * Implements detection, identification, and reading/writing of ATA drives
 * using PIO mode. This class is a singleton.
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
