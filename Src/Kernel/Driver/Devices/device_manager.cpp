#include <Kernel/Driver/Devices/device_manager.h>
#include <Kernel/Driver/Devices/null_device.h>
#include <Kernel/Driver/Devices/zero_device.h>
#include <Kernel/Driver/Devices/serial_device.h>
#include <Kernel/Driver/Devices/console_device.h>

#include <Kernel/Driver/Ata/AtaController.h>

#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/FileSystem/fd.h>

void init_basic_device()
{
    // /Dev/null
    DevFS::the().register_device("null", VNodeType::CharacterDevice, &null_ops, nullptr);

    // /Dev/zero
    DevFS::the().register_device("zero", VNodeType::CharacterDevice, &zero_ops, nullptr);

    // /Dev/ttyS*
    DevFS::the().register_device("ttyS", VNodeType::CharacterDevice, &SerialTTY::ops, nullptr, true);

    // /dev/console
    DevFS::the().register_device("console", VNodeType::CharacterDevice, &ConsoleDevice::ops, nullptr);

    // /dev/ada
    AtaController::the().initialize();

    int fd = fd_open_path("/dev/console", 0);
    if (fd < 0)
        fd = fd_open_path("/dev/ttyS0", 0);

    if (fd >= 0)
    {
        (void)fd_open_path("/dev/console", 0);
        (void)fd_open_path("/dev/console", 0);
    }
}

// TODO/FIXME: Basic device initialization — consider returning error codes if
// device registration or controller initialization fails, and avoid duplicate
// fd_open_path calls. Add clearer ownership semantics and initialization order
// documentation for drivers that depend on these devices.
