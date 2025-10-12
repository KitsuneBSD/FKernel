#include <Kernel/Boot/init.h>
#include <LibFK/Algorithms/log.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/RamFS/ramfs.h>

void init()
{
    auto &vfs = VirtualFS::the();
    auto &ramfs = RamFS::the();

    // Monta o RamFS na raiz
    vfs.mount("/", ramfs.root());

    RetainPtr<VNode> root;
    if (vfs.lookup("/", root) == 0)
    {
        klog("VFS", "Root lookup success: %s", root.get()->m_name.c_str());
    }
    else
    {
        kwarn("VFS", "Root lookup failed");
        return;
    }
}
