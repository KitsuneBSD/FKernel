#pragma once

#include <Kernel/Driver/Ata/AtaConstants.h>
#include <Kernel/Driver/Ata/AtaTypes.h>

class ATAController {
private:
    ATAController() = default;
    ~ATAController() = default;

    ATAController(ATAController const&) = delete;
    ATAController& operator=(ATAController const&) = delete;

    void detect_devices();
    void identify_device(ChannelType channel, DriveType drive);

    void select_drive(ChannelType channel, DriveType drive);
    bool poll_bsy(ChannelType channel);
    bool poll_drq(ChannelType channel);

    ATAChannel channels_[2] = {
        { PRIMARY_IO_BASE, PRIMARY_CTRL_BASE, 14 },
        { SECONDARY_IO_BASE, SECONDARY_CTRL_BASE, 15 }
    };

public:
    static ATAController& instance() noexcept
    {
        static ATAController instance;
        return instance;
    }

    void initialize();
    void handle_irq(ChannelType channel) noexcept;
};
