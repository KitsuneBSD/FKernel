#pragma once

#include <Kernel/Driver/Ata/AtaConstants.h>
#include <Kernel/Driver/Ata/AtaTypes.h>

constexpr int MAX_ATA_DEVICES = 4;

class ATAController {
private:
    ATAController() = default;
    ~ATAController() = default;

    ATAController(ATAController const&) = delete;
    ATAController& operator=(ATAController const&) = delete;

    void detect_devices();
    bool identify_device(ChannelType channel, DriveType drive);

    void select_drive(ChannelType channel, DriveType drive);
    bool poll_bsy(ChannelType channel, LibC::uint64_t timeout_ticks);
    bool poll_drq(ChannelType channel);

    int device_count_ = 0;

    ATAChannel channels_[2] = {
        { PRIMARY_IO_BASE, PRIMARY_CTRL_BASE, 14 },
        { SECONDARY_IO_BASE, SECONDARY_CTRL_BASE, 15 }
    };

    AtaDeviceInfo devices_[MAX_ATA_DEVICES] {};

public:
    static ATAController& instance() noexcept
    {
        static ATAController instance;
        return instance;
    }

    int get_device_count() const { return device_count_; }
    AtaDeviceInfo const* enumerate_devices() const { return devices_; }

    void initialize();
    void handle_irq(ChannelType channel) noexcept;

    bool read_sector(ChannelType channel, DriveType drive, LibC::uint32_t lba, void* buffer);
    bool write_sector(ChannelType channel, DriveType drive, LibC::uint32_t lba, void const* buffer);

    bool read_sectors(ChannelType channel, DriveType drive, LibC::uint32_t lba, LibC::uint8_t sector_count, void* buffer);
    bool write_sectors(ChannelType channel, DriveType drive, LibC::uint32_t lba, LibC::uint8_t sector_count, void const* buffer);
};
