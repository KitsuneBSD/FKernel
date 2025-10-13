#include <Kernel/Driver/Devices/device_manager.h>
#include <Kernel/Driver/Devices/null_device.h>
#include <Kernel/Driver/Devices/zero_device.h>

#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/DevFS/devfs.h>

void init_basic_device()
{
    // /Dev/null
    DevFS::the().register_device("null", VNodeType::CharacterDevice, &null_ops, nullptr);
    klog("DEVFS", "/dev/null registered");

    // /Dev/zero
    DevFS::the().register_device("zero", VNodeType::CharacterDevice, &zero_ops, nullptr);
    klog("DEVFS", "/dev/zero registered");
}