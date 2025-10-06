#include <Kernel/FileSystem/FilesystemManager.h>
#include <Kernel/FileSystem/Filesystem.h>

void FileSystemManager::register_fs(FileSystemDriver *driver)
{
    m_filesystem_drivers.push_back(driver);
}

RetainPtr<VNode> FileSystemManager::mount_fs(const char *fs_name, BlockDevice *device, const char *mount_point, RetainPtr<VNode> root)
{
    for (auto drv : m_filesystem_drivers)
    {
        if (strcmp(drv->m_name, fs_name) == 0)
        {
            auto vnode = drv->mount(device);
            if (!vnode)
            {
                return RetainPtr<VNode>();
            }

            if (root->m_type == VNodeType::Directory)
                root->add_child(vnode);

            return vnode;
        }
    }
    return RetainPtr<VNode>();
}
