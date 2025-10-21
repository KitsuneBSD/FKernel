#include <Kernel/FileSystem/fd.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <LibFK/Algorithms/log.h>

FDTable::FDTable()
{
    m_fds.clear();
}

FDTable &FDTable::the()
{
    static FDTable instance;
    return instance;
}

int FDTable::allocate(RetainPtr<VNode> vnode, int flags)
{
    if (m_fds.is_full())
        return -1;

    FileDescriptor fd;
    fd.fd = -1;
    fd.vnode = vnode;
    fd.flags = flags;
    fd.offset = 0;
    fd.used = true;

    if (!m_fds.push_back(fd))
        return -1;

    int idx = static_cast<int>(m_fds.size()) - 1;
    m_fds[idx].fd = idx;

    klog("FD", "Allocated fd %d for vnode '%s'", idx, vnode->m_name.c_str());
    return idx;
}

FileDescriptor *FDTable::get(int fd)
{
    if (fd < 0 || static_cast<size_t>(fd) >= m_fds.size())
        return nullptr;
    FileDescriptor &f = m_fds[fd];
    return f.used ? &f : nullptr;
}

int FDTable::close(int fd)
{
    FileDescriptor *f = get(fd);
    if (!f)
        return -1;
    if (f->vnode)
        f->vnode->close();
    f->used = false;
    klog("FD", "Closed fd %d", fd);
    return 0;
}

int fd_open_path(const char *path, int flags)
{
    RetainPtr<VNode> vnode;
    int ret = VirtualFS::the().open(path, flags, vnode);
    if (ret < 0)
        return ret;
    return FDTable::the().allocate(vnode, flags);
}

int fd_read(int fd, void *buf, size_t sz)
{
    FileDescriptor *f = FDTable::the().get(fd);
    if (!f || !f->vnode)
        return -1;
    int ret = f->vnode->read(buf, sz, f->offset);
    if (ret > 0)
        f->offset += static_cast<uint64_t>(ret);
    return ret;
}

int fd_write(int fd, const void *buf, size_t sz)
{
    FileDescriptor *f = FDTable::the().get(fd);
    if (!f || !f->vnode)
        return -1;
    int ret = f->vnode->write(buf, sz, f->offset);
    if (ret > 0)
        f->offset += static_cast<uint64_t>(ret);
    return ret;
}

int fd_close(int fd)
{
    return FDTable::the().close(fd);
}
