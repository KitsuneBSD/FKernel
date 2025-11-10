#pragma once

#include <LibFK/Types/types.h>

// ATA I/O Ports
#define ATA_PRIMARY_BASE    0x1F0
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_SECONDARY_BASE  0x170
#define ATA_SECONDARY_CTRL  0x376

// ATA Registers
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT   0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07

// ATA Commands
#define ATA_CMD_READ_PIO    0x20
#define ATA_CMD_WRITE_PIO   0x30
#define ATA_CMD_IDENTIFY    0xEC

// ATA Status Register Bits
#define ATA_STATUS_ERR     0x01 // Error
#define ATA_STATUS_DRQ     0x08 // Data Request Ready
#define ATA_STATUS_SRV     0x10 // Service Request
#define ATA_STATUS_DF      0x20 // Drive Write Fault
#define ATA_STATUS_RDY     0x40 // Ready
#define ATA_STATUS_BSY     0x80 // Busy

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