#include <Kernel/Driver/Devices/device_manager.h>
#include <Kernel/Driver/Devices/null_device.h>
#include <Kernel/Driver/Devices/zero_device.h>
#include <Kernel/Driver/Devices/serial_device.h>
#include <Kernel/Driver/Devices/console_device.h>

#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/DevFS/devfs.h>

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
}