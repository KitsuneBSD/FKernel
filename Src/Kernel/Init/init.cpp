#include <Kernel/Boot/init.h>
#include <LibFK/Algorithms/log.h>

#include <Kernel/Driver/Devices/device_manager.h>

#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/RamFS/ramfs.h>
#include <Kernel/FileSystem/DevFS/devfs.h>

#include <Kernel/Driver/Ata/AtaController.h>

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
}

// TODO/FIXME: High-level init routine — document ordering constraints and error
// handling. Consider returning an error code on failure instead of void so callers can
// make decisions. Also review concurrency/initialization races and ensure subsystems are
// initialized deterministically across different boot paths.
