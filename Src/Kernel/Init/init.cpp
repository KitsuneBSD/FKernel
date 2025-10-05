#include <Kernel/Boot/init.h>

#include <Kernel/Driver/NullDevice/null_device.h>

#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/Posix/unistd.h>
#include <LibFK/Algorithms/log.h>

void pseudo_device_initializer()
{
    devfs_register("null", FileType::CharDevice, &null_ops);
}

void init()
{
    klog("INIT", "Start init");

    VFS::init();
    fdtable_init();

    devfs_init();
    pseudo_device_initializer();
}
