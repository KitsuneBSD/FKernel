#include <Kernel/FileSystem/VirtualFS/vfs.h>

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
    {
        kwarn("VFS", "resolve_path failed: empty path or root not mounted");
        return nullptr;
    }

    if (path[0] != '/')
    {
        kwarn("VFS", "resolve_path failed: path '%s' is not absolute", path);
        return nullptr; // só suportamos paths absolutos
    }

    RetainPtr<VNode> current = m_root;
    klog("VFS", "Resolving path: '%s'", path);

    const char *p = path;
    while (*p)
    {
        // Pula múltiplos '/'
        while (*p == '/')
            ++p;
        if (!*p)
            break;

        // Encontra o final do próximo componente
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
            return nullptr;
        }
        token.assign(p);

        if (strcmp(token.c_str(), ".") == 0)
        {
            // nada, permanece no current
        }
        else if (strcmp(token.c_str(), "..") == 0)
        {
            if (current->parent)
            {
                klog("VFS", "Going up from '%s' to parent '%s'", current->m_name.c_str(), current->parent->m_name.c_str());
                current = current->parent;
            }
            else
            {
                klog("VFS", "Already at root, cannot go up");
            }
        }
        else
        {
            RetainPtr<VNode> next;
            if (current->lookup(token.c_str(), next) != 0)
            {
                kwarn("VFS", "resolve_path failed: component '%s' not found in directory '%s'",
                      token.c_str(), current->m_name.c_str());
                return nullptr; // componente não encontrado
            }
            klog("VFS", "Resolved component '%s' in directory '%s'", token.c_str(), current->m_name.c_str());
            current = next;
        }

        p = end;
    }

    klog("VFS", "resolve_path successful: '%s'", path);
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