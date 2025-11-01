#include <Kernel/FileSystem/file_descriptor.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <LibFK/Algorithms/log.h>

FileDescriptorTable::FileDescriptorTable()
{
    m_file_descriptors.clear();
}

FileDescriptorTable &FileDescriptorTable::the()
{
    static FileDescriptorTable instance;
    return instance;
}

int FileDescriptorTable::allocate(RetainPtr<VNode> vnode, int flags)
{
    if (m_file_descriptors.is_full())
        return -1;

    FileDescriptor file_descriptor;
    file_descriptor.file_descriptor = -1;
    file_descriptor.vnode = vnode;
    file_descriptor.flags = flags;
    file_descriptor.offset = 0;
    file_descriptor.used = true;

    if (!m_file_descriptors.push_back(file_descriptor))
        return -1;

    int idx = static_cast<int>(m_file_descriptors.size()) - 1;
    m_file_descriptors[idx].file_descriptor = idx;

    kdebug("FD", "Allocated file_descriptor %d for vnode '%s'", idx, vnode->m_name.c_str());
    return idx;
}

int FileDescriptorTable::close(int file_descriptor)
{
    FileDescriptor *f = get(file_descriptor);
    if (!f)
        return -1;
    if (f->vnode)
        f->vnode->close();
    f->used = false;

    kdebug("FD", "Closed file_descriptor %d", file_descriptor);
    return 0;
}


FileDescriptor *FileDescriptorTable::get(int file_descriptor)
{
    if (file_descriptor < 0 || static_cast<size_t>(file_descriptor) >= m_file_descriptors.size())
        return nullptr;
    FileDescriptor &f = m_file_descriptors[file_descriptor];
    return f.used ? &f : nullptr;
}

int file_descriptor_open_path(const char *path, int flags)
{
    RetainPtr<VNode> vnode;
    int ret = VirtualFS::the().open(path, flags, vnode);
    if (ret < 0)
        return ret;
    return FileDescriptorTable::the().allocate(vnode, flags);
}

int file_descriptor_read(int file_descriptor, void *buf, size_t sz)
{
    FileDescriptor *f = FileDescriptorTable::the().get(file_descriptor);
    if (!f || !f->vnode)
        return -1;
    int ret = f->vnode->read(buf, sz, f->offset);
    if (ret > 0)
        f->offset += static_cast<uint64_t>(ret);
    return ret;
}

int file_descriptor_write(int file_descriptor, const void *buf, size_t sz)
{
    FileDescriptor *f = FileDescriptorTable::the().get(file_descriptor);
    if (!f || !f->vnode)
        return -1;
    int ret = f->vnode->write(buf, sz, f->offset);
    if (ret > 0)
        f->offset += static_cast<uint64_t>(ret);
    return ret;
}

int file_descriptor_close(int file_descriptor)
{
    return FileDescriptorTable::the().close(file_descriptor);
}
