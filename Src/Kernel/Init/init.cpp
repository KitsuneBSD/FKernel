#include <Kernel/Boot/init.h>
#include <LibFK/Algorithms/log.h>
#include <Kernel/FileSystem/VirtualFS/Vfs.h>

void init()
{
    auto &vfs = VirtualFS::the();

    RetainPtr<VNode> root = make_retain<VNode>();
    root->m_name = "/";
    root->type = VNodeType::Directory;

    vfs.mount("/", root);

    RetainPtr<VNode> result;
    if (vfs.lookup("/", result) == 0)
    {
        klog("VFS", "Root lookup success: %s", result->m_name.c_str());
    }
    else
    {
        kwarn("VFS", "Root lookup failed");
    }
}
