#include <Kernel/Fs/Disk/Ufs/ufs_node.h>
#include <Kernel/Fs/Disk/Ufs/ufs_fs.h>
#include <Kernel/Fs/Disk/Ufs/ufs_super.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

using fk::core::Error;

UfsNode::UfsNode(fk::RefPtr<UfsFileSystem> fs, uint32_t ino, uint64_t size, uint16_t mode)
    : m_fs(fs), m_ino(ino), m_size(size), m_mode(mode) {}

fk::core::Result<size_t, Error>
UfsNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
    return m_fs->read_from_ino(m_ino, offset, size, buffer);
}

fk::core::Result<size_t, Error>
UfsNode::write(uint64_t, size_t, const uint8_t*) {
    return Error::NotImplemented;
}

fk::core::Result<fk::RefPtr<Node>, Error>
UfsNode::lookup(const char* name) {
    return m_fs->lookup_in_ino(m_ino, m_size, name);
}

fk::core::Result<void, Error>
UfsNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    return m_fs->list_dir_ino(m_ino, m_size, entries);
}

fk::core::Result<fk::text::String, Error>
UfsNode::read_link() {
    if (!is_symlink())
        return Error::InvalidData;

    // Fast (inline) symlinks: data is stored in the inode's block pointer area
    // rather than in a real data block. UFS1: up to 60 bytes; UFS2: up to 120.
    if (!m_fs->m_info.is_ufs2) {
        Ufs1Dinode inode;
        auto ir = m_fs->read_inode1(m_ino, inode);
        if (!ir.is_error() && inode.di_blocks == 0 && m_size > 0 && m_size <= 60) {
            char buf[61];
            fk::memory::copy(buf, reinterpret_cast<const char*>(inode.di_db), static_cast<size_t>(m_size));
            buf[m_size] = '\0';
            return fk::text::String(buf);
        }
    } else {
        Ufs2Dinode inode;
        auto ir = m_fs->read_inode2(m_ino, inode);
        if (!ir.is_error() && inode.di_blocks == 0 && m_size > 0 && m_size <= 120) {
            char buf[121];
            fk::memory::copy(buf, reinterpret_cast<const char*>(inode.di_db), static_cast<size_t>(m_size));
            buf[m_size] = '\0';
            return fk::text::String(buf);
        }
    }

    // Normal (block-backed) symlink
    char buf[1024];
    size_t to_read = m_size < sizeof(buf) - 1 ? static_cast<size_t>(m_size) : sizeof(buf) - 1;
    auto res = m_fs->read_from_ino(m_ino, 0, to_read, reinterpret_cast<uint8_t*>(buf));
    if (res.is_error()) return res.error();
    buf[res.value()] = '\0';
    return fk::text::String(buf);
}

} // namespace fkernel
