#include <Kernel/Arch/x86_64/Hardware/Io.h>
#include <Kernel/Arch/x86_64/Hardware/Io_Constants.h>
#include <Kernel/Arch/x86_64/Interrupts/Routines.h>
#include <Kernel/Devices/Storage/BlockDevice.h>
#include <Kernel/Devices/Storage/BlockDeviceTypes.h>
#include <Kernel/Driver/Ata/AtaController.h>
#include <Kernel/Driver/Ata/AtaDevice.h>
#include <Kernel/Driver/Ata/AtaTypes.h>
#include <Kernel/FileSystem/DevFS/DevFS.h>
#include <Kernel/FileSystem/VFS/VFSTypes.h>
#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <LibC/stdint.h>
#include <LibFK/enforce.h>
#include <LibFK/log.h>
#include <LibFK/new.h>

void ATAController::initialize()
{
    detect_devices();
    Log(LogLevel::INFO, "ATAController: Initialization complete");
}

void ATAController::detect_devices()
{
    device_count_ = 0;

    for (int ch = 0; ch < 2; ++ch) {
        for (int dr = 0; dr < 2; ++dr) {
            ChannelType channel = static_cast<ChannelType>(ch);
            DriveType drive = static_cast<DriveType>(dr);

            if (!identify_device(channel, drive))
                continue;

            if (device_count_ >= MAX_ATA_DEVICES)
                continue;

            devices_[device_count_].channel = channel;
            devices_[device_count_].drive = drive;
            devices_[device_count_].present = true;

            void* vnode_mem = Falloc(sizeof(FileSystem::VNode));
            FK::enforce(vnode_mem != nullptr, "Failed to allocate memory for VNode");
            auto* vnode = new (vnode_mem) FileSystem::VNode();

            // Aloca e constrói o BlockDevice
            void* bdev_mem = Falloc(sizeof(Device::BlockDevice));
            FK::enforce(bdev_mem != nullptr, "Failed to allocate memory for BlockDevice");
            auto* bdev = new (bdev_mem) Device::BlockDevice();

            static char names[MAX_ATA_DEVICES][8];
            LibC::snprintf(names[device_count_], sizeof(names[device_count_]), "ada%d", device_count_);

            bdev->name = names[device_count_];
            bdev->block_size = 512;
            bdev->block_count = 0; // opcional: pode tentar extrair do identify_buffer
            bdev->private_data = &devices_[device_count_];
            bdev->ops = nullptr;

            vnode->stat.type = FileSystem::VNodeType::Device;
            vnode->private_data = bdev;
            vnode->ops = nullptr;

            FileSystem::devfs_register_device(bdev->name, vnode);

            Logf(LogLevel::INFO, "DevFS: Registered /dev/%s (ATA device)", bdev->name);

            ++device_count_;
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

bool ATAController::identify_device(ChannelType channel, DriveType drive)
{
    select_drive(channel, drive);
    poll_bsy(channel, 100);

    Io::io_wait();
    Io::outb(channels_[static_cast<int>(channel)].io_base + ATA_REG_SECCOUNT, 0);
    Io::outb(channels_[static_cast<int>(channel)].io_base + ATA_REG_LBA_LOW, 0);
    Io::outb(channels_[static_cast<int>(channel)].io_base + ATA_REG_LBA_MID, 0);
    Io::outb(channels_[static_cast<int>(channel)].io_base + ATA_REG_LBA_HIGH, 0);
    Io::outb(channels_[static_cast<int>(channel)].io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    if (!poll_bsy(channel, 100)) {
        Logf(LogLevel::WARN, "ATA identify_device: Timeout on channel %d drive %d", channel, drive);
        return false;
    }

    auto status = Io::inb(channels_[static_cast<int>(channel)].io_base + ATA_REG_STATUS);
    if (status == 0) {
        return false;
    }

    auto lba_mid = Io::inb(channels_[static_cast<int>(channel)].io_base + ATA_REG_LBA_MID);
    auto lba_high = Io::inb(channels_[static_cast<int>(channel)].io_base + ATA_REG_LBA_HIGH);
    if (lba_mid != 0 || lba_high != 0) {
        return false;
    }

    if (!poll_drq(channel)) {
        return false;
    }

    LibC::uint16_t identify_buffer[256];
    for (int i = 0; i < 256; ++i) {
        identify_buffer[i] = Io::inw(channels_[static_cast<int>(channel)].io_base + ATA_REG_DATA);
    }

    Logf(LogLevel::INFO, "ATA device found at channel %d drive %d", channel, drive);
    return true;
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

bool ATAController::write_sectors(ChannelType channel, DriveType drive, LibC::uint32_t lba, LibC::uint8_t sector_count, void const* buffer)
{
    if (buffer == nullptr || sector_count == 0)
        return false;

    ATAChannel const& ch = channels_[static_cast<int>(channel)];
    select_drive(channel, drive);

    Io::outb(ch.io_base + 2, sector_count);
    Io::outb(ch.io_base + 3, static_cast<LibC::uint8_t>(lba & 0xFF));
    Io::outb(ch.io_base + 4, static_cast<LibC::uint8_t>((lba >> 8) & 0xFF));
    Io::outb(ch.io_base + 5, static_cast<LibC::uint8_t>((lba >> 16) & 0xFF));

    LibC::uint8_t drive_head = 0xE0 | (drive == DriveType::Slave ? 0x10 : 0x00) | ((lba >> 24) & 0x0F);
    Io::outb(ch.io_base + 6, drive_head);

    Io::outb(ch.io_base + 7, 0x30); // WRITE SECTORS command

    auto data = reinterpret_cast<LibC::uint16_t const*>(buffer);

    for (LibC::uint8_t sector = 0; sector < sector_count; ++sector) {
        if (!poll_bsy(channel))
            return false;
        if (!poll_drq(channel))
            return false;

        for (int i = 0; i < 256; ++i) {
            Io::outw(ch.io_base, data[i]);
        }
        data += 256;
    }

    Io::outb(ch.io_base + 7, 0xE7); // FLUSH CACHE command

    if (!poll_bsy(channel))
        return false;

    return true;
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
