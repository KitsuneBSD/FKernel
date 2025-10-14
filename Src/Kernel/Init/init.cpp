#include <Kernel/Boot/init.h>
#include <LibFK/Algorithms/log.h>

#include <Kernel/Driver/Devices/device_manager.h>

#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/RamFS/ramfs.h>
#include <Kernel/FileSystem/DevFS/devfs.h>

void test_console_write()
{
    VNode *root = DevFS::the().root().get();

    RetainPtr<VNode> Console;
    root->lookup("console", Console);

    const char *msg = "Hello, FKernel!\n";
    Console->ops->write(Console.get(), msg, strlen(msg), 0);
}

void init()
{
    auto &vfs = VirtualFS::the();
    auto &ramfs = RamFS::the();
    auto &devfs = DevFS::the();
    klog("INIT", "Start init");

    // Monta RamFS em /
    vfs.mount("/", ramfs.root());

    RetainPtr<VNode> root;
    if (vfs.lookup("/", root) != 0)
    {
        kwarn("VFS", "Root lookup failed");
        return;
    }

    vfs.mount("dev", devfs.root());

    RetainPtr<VNode> dev;
    vfs.lookup("/dev", dev);

    init_basic_device();

    test_console_write();
}
