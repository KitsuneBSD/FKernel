#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/FileSystem/DevFS/device.h>
#include <Kernel/FileSystem/file_descriptor.h>

#include <LibC/string.h>

DevFS::DevFS()
{
    VNode *root_node = new VNode();
    root_node->m_name = "/dev";
    root_node->type = VNodeType::Directory;
    root_node->inode = new Inode(2);
    root_node->ops = &ops;
    d_root = adopt_retain(root_node);
}

DevFS &DevFS::the()
{
    static DevFS instance;
    return instance;
}

RetainPtr<VNode> DevFS::root()
{
    return d_root;
}

int DevFS::register_device(const char *prefix, VNodeType type, VNodeOps *ops, void *driver_data, bool has_multiple)
{
    int index = 0;
    for (auto &dev : d_devices)
    {
        if (strncmp(dev.d_name.c_str(), prefix, strlen(prefix)) == 0)
            ++index;
    }

    char name_buf[64];

    snprintf(name_buf, sizeof(name_buf), "%s", prefix);

    if (has_multiple)
    {
        snprintf(name_buf, sizeof(name_buf), "%s%d", prefix, index);
    }

    return register_device_static(name_buf, type, ops, driver_data);
}

int DevFS::register_device_static(const char *name, VNodeType type, VNodeOps *ops, void *driver_data)
{
    if ((type != VNodeType::CharacterDevice) && (type != VNodeType::BlockDevice))
    {
        kwarn("DEVFS", "To register in a devfs we need the type of VNode be a character device or a block device");
        return -1;
    }

    if (d_devices.size() >= d_devices.capacity())
    {
        kwarn("DEVFS", "Device table full, cannot register '%s'", name);
        return -1;
    }

    Device dev;
    dev.d_name = name;
    dev.d_type = type;
    dev.ops = ops;
    dev.driver_data = driver_data;
    d_devices.push_back(dev);

    auto dev_node = adopt_retain(new VNode());
    dev_node->m_name = name;
    dev_node->type = type;
    dev_node->parent = d_root.get();
    dev_node->inode = new Inode(d_root->inode_number + d_devices.size());
    dev_node->inode_number = dev_node->inode->i_number;
    dev_node->fs_private = driver_data;
    dev_node->ops = ops;

    d_root->dir_entries.push_back(DirEntry{name, dev_node});

    klog("DEVFS", "Registered device '%s' (type=%d) at %p", name, (int)type, driver_data);
    return 0;
}

int DevFS::unregister_device(const char *name)
{
    for (size_t i = 0; i < d_devices.size(); ++i)
    {
        if (strcmp(d_devices[i].d_name.c_str(), name) == 0)
        {
            d_devices.erase(i);

            for (size_t j = 0; j < d_root->dir_entries.size(); ++j)
            {
                if (strcmp(d_root->dir_entries[j].m_name.c_str(), name) == 0)
                {
                    d_root->dir_entries.erase(j);
                    break;
                }
            }

            klog("DEVFS", "Unregistered device '%s'", name);
            return 0;
        }
    }

    kwarn("DEVFS", "Unregister_device: '%s' not found", name);
    return -1;
}

int DevFS::devfs_lookup(VNode *vnode, FileDescriptor *fd, const char *name, RetainPtr<VNode> &out)
{
    (void)fd;
    if (!vnode || vnode->type != VNodeType::Directory)
        return -1;

    for (auto &entry : vnode->dir_entries)
    {
        if (strcmp(entry.m_name.c_str(), name) == 0)
        {
            out = entry.m_vnode;
            return 0;
        }
    }

    kwarn("DEVFS", "Lookup: device '%s' not found", name);
    return -1;
}

int DevFS::devfs_readdir(VNode *vnode, FileDescriptor *fd, void *buffer, size_t max_entries)
{
    (void)fd;
    if (!vnode || vnode->type != VNodeType::Directory)
        return -1;

    DirEntry *buf = reinterpret_cast<DirEntry *>(buffer);
    size_t n = vnode->dir_entries.size();
    if (n > max_entries)
        n = max_entries;

    for (size_t i = 0; i < n; ++i)
        buf[i] = vnode->dir_entries[i];

    return static_cast<int>(n);
}

int DevFS::devfs_open(VNode *vnode, FileDescriptor *fd, int flags)
{
    (void)fd;
    if (vnode->ops && vnode->ops->open)
        return vnode->ops->open(vnode, fd, flags);
    return 0;
}

int DevFS::devfs_close(VNode *vnode, FileDescriptor *fd)
{
    (void)fd;
    if (vnode->ops && vnode->ops->close)
        return vnode->ops->close(vnode, fd);
    return 0;
}

int DevFS::devfs_read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size, size_t offset)
{(void)fd;
    if (vnode->ops && vnode->ops->read)
        return vnode->ops->read(vnode, fd, buffer, size, offset);
    return -1;
}

int DevFS::devfs_write(VNode *vnode, FileDescriptor *fd, const void *buffer, size_t size, size_t offset)
{
    (void)fd;
    if (vnode->ops && vnode->ops->write)
        return vnode->ops->write(vnode, fd, buffer, size, offset);
    return -1;
}

int DevFS::devfs_create(VNode *dir, FileDescriptor *fd, const char *name, VNodeType type, RetainPtr<VNode> &out)
{
    (void)fd;
    if (!dir || dir->type != VNodeType::Directory)
        return -1;

    auto new_node = adopt_retain(new VNode());
    new_node->m_name = name;
    new_node->type = type;
    new_node->parent = dir;
    new_node->inode = new Inode(dir->inode_number + dir->dir_entries.size() + 1);
    new_node->ops = &ops;
    dir->dir_entries.push_back(DirEntry{name, new_node});

    out = new_node; // importante!

    klog("DEVFS", "Created node '%s' in %s", name, dir->m_name.c_str());
    return 0;
}

int DevFS::devfs_unlink(VNode *dir, FileDescriptor *fd, const char *name)
{
    (void)fd;
    if (!dir || dir->type != VNodeType::Directory)
        return -1;

    for (size_t i = 0; i < dir->dir_entries.size(); ++i)
    {
        if (strcmp(dir->dir_entries[i].m_name.c_str(), name) == 0)
        {
            dir->dir_entries.erase(i);
            klog("DEVFS", "Unlinked '%s' from %s", name, dir->m_name.c_str());
            return 0;
        }
    }

    kwarn("DEVFS", "Unlink: '%s' not found in %s", name, dir->m_name.c_str());
    return -1;
}

VNodeOps DevFS::ops = {
    .read = &DevFS::devfs_read,
    .write = &DevFS::devfs_write,
    .open = &DevFS::devfs_open,
    .close = &DevFS::devfs_close,
    .lookup = &DevFS::devfs_lookup,
    .create = &DevFS::devfs_create,
    .readdir = &DevFS::devfs_readdir,
    .unlink = &DevFS::devfs_unlink,
};