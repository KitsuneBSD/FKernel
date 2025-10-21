#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <LibC/string.h>
#include <LibC/stdio.h>
#include <LibFK/Algorithms/log.h>

RetainPtr<VNode> VirtualFS::root() const
{
    return v_root;
}

int VirtualFS::mount(const char *name, RetainPtr<VNode> root)
{
    if (v_mounts.is_full())
    {
        kwarn("VFS", "Failed to mount '%s': mount table full", name);
        return -1;
    }

    Mountpoint m(name, root);

    if (!v_root)
    {
        v_root = root;  // primeiro mount se torna '/'
        v_cwd = v_root; // inicializa cwd
        klog("VFS", "Root mounted at '/'");
    }
    else
    {
        v_root->dir_entries.push_back(DirEntry{name, root});
        klog("VFS", "Mounted '%s' at '/%s' successfully", name, name);
    }

    v_mounts.push_back(m);
    klog("VFS", "Mounted '%s' successfully", name);
    return 0;
}

// Resolve path absoluto ou relativo ao cwd
RetainPtr<VNode> VirtualFS::resolve_path(const char *path, RetainPtr<VNode> cwd)
{
    if (!path || !*path || !v_root)
    {
        kwarn("VFS", "Resolve path failed: empty path or root not mounted");
        return RetainPtr<VNode>();
    }

    RetainPtr<VNode> current;

    if (path[0] == '/')
    {
        current = v_root; // absoluto ignora cwd
    }
    else
    {
        current = cwd ? cwd : v_cwd; // relativo a cwd
    }

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

        token.assign(p, len);

        if (strcmp(token.c_str(), ".") == 0)
        {
            // não faz nada
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

// Define ou retorna o cwd atual
RetainPtr<VNode> VirtualFS::cwd() const
{
    return v_cwd;
}

void VirtualFS::set_cwd(RetainPtr<VNode> vnode)
{
    if (vnode)
        v_cwd = vnode;
}

// Lookup ajustado para suportar cwd
int VirtualFS::lookup(const char *path, RetainPtr<VNode> &out)
{
    RetainPtr<VNode> vnode = resolve_path(path, v_cwd);
    if (!vnode)
    {
        kwarn("VFS", "Lookup failed: path '%s' not found", path);
        return -1;
    }
    out = vnode;
    klog("VFS", "Lookup: path '%s' found", path);
    return 0;
}

// Open ajustado para suportar cwd
int VirtualFS::open(const char *path, int flags, RetainPtr<VNode> &out)
{
    RetainPtr<VNode> vnode = resolve_path(path, v_cwd);
    if (!vnode)
    {
        kwarn("VFS", "Open failed: path '%s' not found", path);
        return -1;
    }

    int ret = vnode->open(flags);
    out = vnode;
    return ret;
}

// Read ajustado para suportar cwd
int VirtualFS::read(const char *path, void *buf, size_t sz, size_t off)
{
    RetainPtr<VNode> vnode = resolve_path(path, v_cwd);
    if (!vnode)
    {
        kwarn("VFS", "Read failed: path '%s' not found", path);
        return -1;
    }

    int ret = vnode->read(buf, sz, off);
    if (ret < 0)
        kwarn("VNode", "Read op on '%s' returned error %d", vnode->m_name.c_str(), ret);
    else
        klog("VNode", "Read %d bytes from '%s' (offset=%zu)", ret, vnode->m_name.c_str(), off);

    return ret;
}

// Write ajustado para suportar cwd
int VirtualFS::write(const char *path, const void *buf, size_t sz, size_t off)
{
    RetainPtr<VNode> vnode = resolve_path(path, v_cwd);
    if (!vnode)
    {
        kwarn("VFS", "Write failed: path '%s' not found", path);
        return -1;
    }

    int ret = vnode->write(buf, sz, off);
    if (ret < 0)
        kwarn("VNode", "Write op on '%s' returned error %d", vnode->m_name.c_str(), ret);
    else
        klog("VNode", "Wrote %d bytes to '%s' (offset=%zu)", ret, vnode->m_name.c_str(), off);

    return ret;
}