#include <Kernel/FileSystem/VirtualFS/Vfs.h>
#include <LibC/string.h>
#include <LibC/stdio.h>
#include <LibFK/Algorithms/log.h>

int VirtualFS::mount(const char *name, RetainPtr<VNode> root)
{
    if (m_mounts.is_full())
    {
        kwarn("VFS", "Failed to mount '%s': mount table full", name);
        return -1;
    }

    Mountpoint m;
    m.m_name = name;
    m.m_root = root;

    if (!m_root)
    {
        m_root = root; // First mount becomes root '/'
    }

    m_mounts.push_back(m);
    klog("VFS", "Mounted '%s' successfully", name);
    return 0;
}

RetainPtr<VNode> VirtualFS::root()
{
    return m_root;
}

VNode *VirtualFS::resolve_path(const char *path)
{
    if (!path || !*path || !m_root)
        return nullptr;

    if (path[0] != '/')
        return nullptr;

    RetainPtr<VNode> current = m_root;
    char temp[128];
    strncpy(temp, path + 1, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    char *token = strtok(temp, "/");
    while (token)
    {
        RetainPtr<VNode> next;
        if (current->lookup(token, next) != 0)
            return nullptr;
        current = next;
        token = strtok(nullptr, "/");
    }

    return current.get();
}

int VirtualFS::lookup(const char *path, RetainPtr<VNode> &out)
{
    auto vnode = resolve_path(path);
    if (!vnode)
    {
        kwarn("VFS", "lookup failed: path '%s' not found", path);
        return -1;
    }
    out = vnode;
    klog("VFS", "lookup: path '%s' found", path);
    return 0;
}

int VirtualFS::open(const char *path, int flags, RetainPtr<VNode> &out)
{
    auto vnode = resolve_path(path);
    if (!vnode)
    {
        kwarn("VFS", "open failed: path '%s' not found", path);
        return -1;
    }
    vnode->open(flags);
    out = vnode;
    klog("VFS", "open: path '%s' opened with flags 0x%x", path, flags);
    return 0;
}

int VirtualFS::read(const char *path, void *buf, size_t sz, size_t off)
{
    auto vnode = resolve_path(path);
    if (!vnode)
    {
        kwarn("VFS", "read failed: path '%s' not found", path);
        return -1;
    }
    int ret = vnode->read(buf, sz, off);
    klog("VFS", "read: %zu bytes from path '%s' at offset %zu", ret, path, off);
    return ret;
}

int VirtualFS::write(const char *path, const void *buf, size_t sz, size_t off)
{
    auto vnode = resolve_path(path);
    if (!vnode)
    {
        kwarn("VFS", "write failed: path '%s' not found", path);
        return -1;
    }
    int ret = vnode->write(buf, sz, off);
    klog("VFS", "write: %d bytes to path '%s' at offset %zu", ret, path, off);
    return ret;
}