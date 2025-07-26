#include "Kernel/Arch/x86_64/Interrupts/Routines.h"
#include "LibFK/enforce.h"
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

bool ATAController::poll_bsy(ChannelType channel, LibC::uint64_t timeout_ticks = 1000)
{
    ATAChannel const& ch = channels_[static_cast<int>(channel)];
    LibC::uint64_t start = uptime();

    while (uptime() - start < timeout_ticks) {
        LibC::uint8_t status = Io::inb(ch.io_base + 7);
        if ((status & 0x80) == 0) {
            return true;
        }
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

bool ATAController::read_sector(ChannelType channel, DriveType drive, LibC::uint32_t lba, void* buffer)
{
    if (FK::alert_if_f(buffer != nullptr, "ATAController::read_sector: buffer cannot be nullptr"))
        return false;

    ATAChannel const& ch = channels_[static_cast<int>(channel)];
    select_drive(channel, drive);

    Io::outb(ch.io_base + 2, 1);                                              // Sector count = 1
    Io::outb(ch.io_base + 3, static_cast<LibC::uint8_t>(lba & 0xFF));         // Sector number (LBA bits 0-7)
    Io::outb(ch.io_base + 4, static_cast<LibC::uint8_t>((lba >> 8) & 0xFF));  // Cylinder low (LBA bits 8-15)
    Io::outb(ch.io_base + 5, static_cast<LibC::uint8_t>((lba >> 16) & 0xFF)); // Cylinder high (LBA bits 16-23)

    // Drive/head register: 0xE0 | drive bit | LBA bits 24-27
    LibC::uint8_t drive_head = 0xE0 | (drive == DriveType::Slave ? 0x10 : 0x00) | ((lba >> 24) & 0x0F);
    Io::outb(ch.io_base + 6, drive_head);

    Io::outb(ch.io_base + 7, 0x20); // READ SECTORS command

    if (!poll_bsy(channel))
        return false;

    if (!poll_drq(channel))
        return false;

    // Read 256 words (512 bytes)
    LibC::uint16_t* data = reinterpret_cast<LibC::uint16_t*>(buffer);
    for (int i = 0; i < 256; ++i) {
        data[i] = Io::inw(ch.io_base);
    }

    return true;
}

bool ATAController::write_sector(ChannelType channel, DriveType drive, LibC::uint32_t lba, void const* buffer)
{
    if (FK::alert_if_f(buffer != nullptr, "ATAController::write_sector: buffer cannot be nullptr"))
        return false;

    ATAChannel const& ch = channels_[static_cast<int>(channel)];
    select_drive(channel, drive);

    Io::outb(ch.io_base + 2, 1); // Sector count = 1
    Io::outb(ch.io_base + 3, static_cast<LibC::uint8_t>(lba & 0xFF));
    Io::outb(ch.io_base + 4, static_cast<LibC::uint8_t>((lba >> 8) & 0xFF));
    Io::outb(ch.io_base + 5, static_cast<LibC::uint8_t>((lba >> 16) & 0xFF));

    LibC::uint8_t drive_head = 0xE0 | (drive == DriveType::Slave ? 0x10 : 0x00) | ((lba >> 24) & 0x0F);
    Io::outb(ch.io_base + 6, drive_head);

    Io::outb(ch.io_base + 7, 0x30); // WRITE SECTORS command

    if (!poll_bsy(channel))
        return false;

    if (!poll_drq(channel))
        return false;

    LibC::uint16_t const* data = reinterpret_cast<LibC::uint16_t const*>(buffer);
    for (int i = 0; i < 256; ++i) {
        Io::outw(ch.io_base, data[i]);
    }

    Io::outb(ch.io_base + 7, 0xE7); // FLUSH CACHE command

    if (!poll_bsy(channel))
        return false;

    return true;
}

bool ATAController::read_sectors(ChannelType channel, DriveType drive, LibC::uint32_t lba, LibC::uint8_t sector_count, void* buffer)
{
    if (FK::alert_if_f(buffer != nullptr, "ATAController::read_sectors: buffer cannot be nullptr"))
        return false;
    if (FK::alert_if_f(sector_count > 0, "ATAController::read_sectors: sector_count must be > 0"))
        return false;

    ATAChannel const& ch = channels_[static_cast<int>(channel)];
    select_drive(channel, drive);

    Io::outb(ch.io_base + 2, sector_count);
    Io::outb(ch.io_base + 3, static_cast<LibC::uint8_t>(lba & 0xFF));
    Io::outb(ch.io_base + 4, static_cast<LibC::uint8_t>((lba >> 8) & 0xFF));
    Io::outb(ch.io_base + 5, static_cast<LibC::uint8_t>((lba >> 16) & 0xFF));

    LibC::uint8_t drive_head = 0xE0 | (drive == DriveType::Slave ? 0x10 : 0x00) | ((lba >> 24) & 0x0F);
    Io::outb(ch.io_base + 6, drive_head);

    Io::outb(ch.io_base + 7, 0x20); // READ SECTORS

    LibC::uint16_t* data = reinterpret_cast<LibC::uint16_t*>(buffer);

    for (LibC::uint8_t sector = 0; sector < sector_count; ++sector) {
        if (!poll_bsy(channel))
            return false;
        if (!poll_drq(channel))
            return false;

        for (int i = 0; i < 256; ++i) {
            data[i] = Io::inw(ch.io_base);
        }
        data += 256;
    }

    return true;
}
