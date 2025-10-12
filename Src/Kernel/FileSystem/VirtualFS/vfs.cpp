#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <LibC/string.h>
#include <LibC/stdio.h>
#include <LibFK/Algorithms/log.h>

RetainPtr<VNode> VirtualFS::root()
{
    return m_root;
}

int VirtualFS::mount(const char *name, RetainPtr<VNode> root)
{
    if (m_mounts.is_full())
    {
        kwarn("VFS", "Failed to mount '%s': mount table full", name);
        return -1;
    }

    Mountpoint m(name, root);

    if (!m_root)
        m_root = root; // first mount becomes '/'

    m_mounts.push_back(m);
    klog("VFS", "Mounted '%s' successfully", name);
    return 0;
}

RetainPtr<VNode> VirtualFS::resolve_path(const char *path)
{
    if (!path || !*path || !m_root)
    {
        kwarn("VFS", "Resolve path failed: empty path or root not mounted");
        return RetainPtr<VNode>();
    }

    if (path[0] != '/')
    {
        kwarn("VFS", "Resolve path failed: path '%s' is not absolute", path);
        return RetainPtr<VNode>();
    }

    RetainPtr<VNode> current = m_root;
    klog("VFS", "Resolving path: '%s'", path);

    const char *p = path;
    while (*p)
    {
        while (*p == '/')
            ++p;
        if (!*p)
            break;

        const char *end = p;
        while (*end && *end != '/')
            ++end;
        size_t len = end - p;
        if (len == 0)
            break;

        fixed_string<256> token;
        if (len >= token.capacity())
        {
            kwarn("VFS", "Path component too long: '%.*s'", int(len), p);
            return RetainPtr<VNode>();
        }
        token.assign(p);

        if (strcmp(token.c_str(), ".") == 0)
        {
            // stay
        }
        else if (strcmp(token.c_str(), "..") == 0)
        {
            if (current->parent)
                current = current->parent;
        }
        else
        {
            RetainPtr<VNode> next;
            if (current->lookup(token.c_str(), next) != 0)
            {
                kwarn("VFS", "Component '%s' not found in '%s'", token.c_str(), current->m_name.c_str());
                return RetainPtr<VNode>();
            }
            current = next;
        }

        p = end;
    }

    klog("VFS", "Resolved path successfully: '%s'", path);
    return current;
}

int VirtualFS::lookup(const char *path, RetainPtr<VNode> &out)
{
    RetainPtr<VNode> vnode = resolve_path(path);
    if (!vnode)
    {
        kwarn("VFS", "Lookup failed: path '%s' not found", path);
        return -1;
    }
    out = vnode;
    klog("VFS", "Lookup: path '%s' found", path);
    return 0;
}

int VirtualFS::open(const char *path, int flags, RetainPtr<VNode> &out)
{
    RetainPtr<VNode> vnode = resolve_path(path);
    if (!vnode)
    {
        kwarn("VFS", "Open failed: path '%s' not found", path);
        return -1;
    }

    int ret = vnode->ops->open(vnode.get(), flags);
    out = vnode;
    return ret;
}

int VirtualFS::read(const char *path, void *buf, size_t sz, size_t off)
{
    RetainPtr<VNode> vnode = resolve_path(path);
    if (!vnode)
    {
        kwarn("VFS", "Read failed: path '%s' not found", path);
        return -1;
    }

    if (!vnode->ops)
    {
        kwarn("VNode", "Read failed: vnode '%s' has null ops", vnode->m_name.c_str());
        return -1;
    }

    if (!vnode->ops->read)
    {
        kwarn("VNode", "Read failed: vnode '%s' has no read operation assigned", vnode->m_name.c_str());
        return -1;
    }

    int ret = vnode->ops->read(vnode.get(), buf, sz, off);
    if (ret < 0)
        kwarn("VNode", "Read op on '%s' returned error %d", vnode->m_name.c_str(), ret);
    else
        klog("VNode", "Read %d bytes from '%s' (offset=%zu)", ret, vnode->m_name.c_str(), off);

    return ret;
}

int VirtualFS::write(const char *path, const void *buf, size_t sz, size_t off)
{
    RetainPtr<VNode> vnode = resolve_path(path);
    if (!vnode)
    {
        kwarn("VFS", "Write failed: path '%s' not found", path);
        return -1;
    }

    if (!vnode->ops)
    {
        kwarn("VNode", "Write failed: vnode '%s' has null ops", vnode->m_name.c_str());
        return -1;
    }

    if (!vnode->ops->write)
    {
        kwarn("VNode", "Write failed: vnode '%s' has no write operation assigned", vnode->m_name.c_str());
        return -1;
    }

    int ret = vnode->ops->write(vnode.get(), buf, sz, off);
    if (ret < 0)
        kwarn("VNode", "Write op on '%s' returned error %d", vnode->m_name.c_str(), ret);
    else
        klog("VNode", "Wrote %d bytes to '%s' (offset=%zu)", ret, vnode->m_name.c_str(), off);

    return ret;
}