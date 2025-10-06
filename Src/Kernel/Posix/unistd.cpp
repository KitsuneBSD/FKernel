#include <LibFK/Container/static_vector.h>
#include <LibFK/Memory/own_ptr.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/Posix/unistd.h>
#include <Kernel/Posix/errno.h>

void FDTable::init()
{
    m_entries.clear();
    klog("FDTable", "File descriptor table initialized");
}

int FDTable::allocate(VFSNode *node)
{
    if (!node)
        return -1;

    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        if (!m_entries[i].node)
        {
            m_entries[i] = FDEntry(node);
            return static_cast<int>(i);
        }
    }

    if (m_entries.size() >= MAX_FD)
        return -1;

    m_entries.push_back(FDEntry(node));
    return static_cast<int>(m_entries.size() - 1);
}

void FDTable::release(int fd)
{
    if (fd < 0 || static_cast<size_t>(fd) >= m_entries.size())
        return;
    m_entries[fd] = FDEntry(); // destrói entry atual, libera node
}

FDEntry *FDTable::get(int fd)
{
    if (fd < 0 || static_cast<size_t>(fd) >= m_entries.size())
        return nullptr;
    if (!m_entries[fd].node)
        return nullptr;
    return &m_entries[fd];
}

int open(const char *pathname, int flags, int /*mode*/)
{
    auto node = VFS::the().resolve_path(pathname);
    if (!node)
        return -1;

    int fd = FDTable::the().allocate(node);
    if (fd < 0)
        return -1;

    if (node->ops)
        node->ops->open(node, static_cast<FileMode>(flags));
    return fd;
}

int close(int fd)
{
    auto entry = FDTable::the().get(fd);
    if (!entry)
        return -1;
    if (entry->node && entry->node->ops)
        entry->node->ops->close(entry->node);

    FDTable::the().release(fd);
    return 0;
}

ssize_t read(int fd, void *buf, size_t count)
{
    auto entry = FDTable::the().get(fd);
    if (!entry || !entry->node || !entry->node->ops)
        return -1;

    ssize_t ret = entry->node->ops->read(entry->node, buf, count, entry->offset);
    if (ret > 0)
        entry->offset += static_cast<size_t>(ret);
    return ret;
}

ssize_t write(int fd, const void *buf, size_t count)
{
    auto entry = FDTable::the().get(fd);
    if (!entry || !entry->node || !entry->node->ops)
        return -1;

    ssize_t ret = entry->node->ops->write(entry->node, buf, count, entry->offset);
    if (ret > 0)
        entry->offset += static_cast<size_t>(ret);
    return ret;
}

off_t lseek(int fd, off_t offset, int whence)
{
    auto entry = FDTable::the().get(fd);
    if (!entry || !entry->node)
        return -1;

    size_t new_offset = 0;
    switch (whence)
    {
    case SEEK_SET:
        new_offset = static_cast<size_t>(offset);
        break;
    case SEEK_CUR:
        new_offset = entry->offset + static_cast<size_t>(offset);
        break;
    case SEEK_END:
        new_offset = entry->node->size + static_cast<size_t>(offset);
        break;
    default:
        return -1;
    }

    entry->offset = new_offset;
    return static_cast<off_t>(entry->offset);
}

int unlink(const char *pathname)
{
    return VFS::the().unlink(pathname);
}

int mkdir(const char *pathname, mode_t mode)
{
    FilePermissions perms{(uint16_t)((mode >> 6) & 7),
                          (uint16_t)((mode >> 3) & 7),
                          (uint16_t)(mode & 7)};
    return VFS::the().mkdir(pathname, perms);
}