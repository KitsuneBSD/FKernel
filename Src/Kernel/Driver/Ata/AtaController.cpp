#include <Kernel/Arch/x86_64/Hardware/Io.h>
#include <Kernel/Arch/x86_64/Hardware/Io_Constants.h>
#include <Kernel/Driver/Ata/AtaController.h>
#include <Kernel/Driver/Ata/AtaTypes.h>
#include <LibC/stdint.h>
#include <LibFK/log.h>

void ATAController::initialize()
{
    detect_devices();
    Log(LogLevel::INFO, "ATAController: Initialization complete");
}

void ATAController::detect_devices()
{
    for (int ch = 0; ch < 2; ++ch) {
        for (int dr = 0; dr < 2; ++dr) {
            identify_device(static_cast<ChannelType>(ch), static_cast<DriveType>(dr));
        }
    }
}

void ATAController::select_drive(ChannelType channel, DriveType drive)
{
    ATAChannel const& ch = channels_[static_cast<int>(channel)];

    LibC::uint8_t drive_head = (drive == DriveType::Master) ? 0xA0 : 0xB0;
    Io::outb(ch.io_base + 6, drive_head);
    Io::io_wait();
}

bool ATAController::poll_bsy(ChannelType channel)
{
    ATAChannel const& ch = channels_[static_cast<int>(channel)];

    for (int i = 0; i < 1'000'000; ++i) {
        LibC::uint8_t status = Io::inb(ch.io_base + 7);
        if ((status & 0x80) == 0)
            return true;
    }

    Logf(LogLevel::TRACE, "ATAController: Timeout polling BSY on channel %d", static_cast<int>(channel));
    return false;
}

bool ATAController::poll_drq(ChannelType channel)
{
    ATAChannel const& ch = channels_[static_cast<int>(channel)];
    for (int i = 0; i < 1'000'000; ++i) {
        LibC::uint8_t status = Io::inb(ch.io_base + 7);
        if (status & 0x08)
            return true;
    }
    Logf(LogLevel::TRACE, "ATAController: Timeout polling DRQ on channel %d", static_cast<int>(channel));
    return false;
}

void ATAController::identify_device(ChannelType channel, DriveType drive)
{
    ATAChannel const& ch = channels_[static_cast<int>(channel)];

    select_drive(channel, drive);

    Io::outb(ch.io_base + 1, 0);
    Io::outb(ch.io_base + 2, 0);
    Io::outb(ch.io_base + 3, 0);
    Io::outb(ch.io_base + 4, 0);
    Io::outb(ch.io_base + 5, 0);
    Io::outb(ch.io_base + 7, 0xEC);

    Io::io_wait();

    LibC::uint8_t status = Io::inb(ch.io_base + 7);
    if (status == 0) {
        Logf(LogLevel::TRACE, "ATAController: No device present at channel %d, drive %d", static_cast<int>(channel), static_cast<int>(drive));
        return;
    }

    if (!poll_bsy(channel) || !poll_drq(channel))
        return;

    LibC::uint16_t identify_data[256];
    for (int i = 0; i < 256; ++i) {
        identify_data[i] = Io::inw(ch.io_base);
    }

    char model[41] = { 0 };
    for (int i = 0; i < 20; ++i) {
        model[i * 2] = (identify_data[27 + i] >> 8) & 0xFF;
        model[i * 2 + 1] = identify_data[27 + i] & 0xFF;
    }

    Logf(LogLevel::INFO, "ATAController: Detected device on channel %d drive %d - Model: %s", static_cast<int>(channel), static_cast<int>(drive), model);
}

void ATAController::handle_irq(ChannelType channel) noexcept
{
    ATAChannel const& ch = channels_[static_cast<int>(channel)];
    LibC::uint8_t status = Io::inb(ch.io_base + 7);

    if (status & 0x01) {
        Logf(LogLevel::ERROR, "ATAController: IRQ Error on channel %d", static_cast<int>(channel));
        return;
    }

    if (status & 0x08) {
        Logf(LogLevel::TRACE, "ATAController: IRQ DRQ set on channel %d", static_cast<int>(channel));
    }
}
