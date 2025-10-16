#pragma once

#include <LibC/stdint.h>
#include <LibC/stdbool.h>

#include <Kernel/Driver/Ata/AtaBlockDevice.h>

#define ATA_PRIMARY_BASE 0x1F0
#define ATA_SECONDARY_BASE 0x170
#define ATA_PRIMARY_CTRL 0x3F6
#define ATA_SECONDARY_CTRL 0x376

#define ATA_REG_DATA 0x00
#define ATA_REG_STATUS 0x07
#define ATA_REG_COMMAND 0x07
#define ATA_REG_SECCOUNT 0x02
#define ATA_REG_LBA0 0x03
#define ATA_REG_LBA1 0x04
#define ATA_REG_LBA2 0x05
#define ATA_REG_HDDEVSEL 0x06

#define ATA_CMD_IDENTIFY 0xEC
#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_BUSY 0x80

enum class Bus
{
    Primary,
    Secondary
};

enum class Drive
{
    Master,
    Slave
};

struct AtaDeviceInfo
{
    Bus bus;
    Drive drive;
    bool exists;
    char model[41];
};

class AtaController
{
public:
    static AtaController &the();
    void initialize();

    int read_sectors_pio(const AtaDeviceInfo &device, uint32_t lba, uint8_t count, void *buffer);
    int write_sectors_pio(const AtaDeviceInfo &device, uint32_t lba, uint8_t count, const void *buffer);

private:
    AtaController() = default;

    void detect_devices();
    bool identify_device(Bus bus, Drive drive, AtaDeviceInfo &out);

    uint16_t base_io(Bus bus);
    uint16_t ctrl_io(Bus bus);
    int read_sectors_pio(Bus bus, Drive drive, uint32_t lba, uint8_t sector_count, void *buffer);
    int write_sectors_pio(Bus bus, Drive drive, uint32_t lba, uint8_t sector_count, const void *buffer);
};