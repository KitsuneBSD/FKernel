#include <Kernel/Fs/Disk/Ext4/ext4_node.h>
#include <Kernel/Fs/Disk/Ext4/ext4_fs.h>

namespace fkernel {

using fk::core::Error;
using fk::core::Result;

Ext4Node::Ext4Node(fk::RefPtr<Ext4FileSystem> fs, uint32_t ino, const Ext2Inode& inode)
    : m_fs(fk::types::move(fs)), m_ino(ino), m_inode(inode)
{
    m_is_dir = ((inode.i_mode & 0xF000u) == 0x4000u);
}

Result<size_t, Error> Ext4Node::read(uint64_t offset, size_t size, uint8_t* buffer) {
    if (m_is_dir) return Error::IsDirectory;
    return m_fs->read_from_inode_ext4(m_inode, offset, size, buffer);
}

Result<size_t, Error> Ext4Node::write(uint64_t /*offset*/, size_t /*size*/, const uint8_t* /*buffer*/) {
    // Extent-tree write allocation not yet implemented; indirect-block files could
    // use m_ext2->write path but we can't distinguish safely here without re-reading
    // i_flags. Return NotImplemented for now.
    return Error::NotImplemented;
}

Result<fk::RefPtr<Node>, Error> Ext4Node::lookup(const char* name) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->lookup_in_inode_ext4(m_inode, m_ino, name);
}

Result<void, Error>
Ext4Node::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->list_dir_inode_ext4(m_inode, entries);
}

Result<fk::RefPtr<Node>, Error> Ext4Node::create_child(const char* name, int mode) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->m_ext2->create_child(name, mode);
}

Result<fk::RefPtr<Node>, Error> Ext4Node::mkdir(const char* name, int mode) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->m_ext2->mkdir(name, mode);
}

Result<void, Error> Ext4Node::unlink(const char* name) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->m_ext2->unlink(name);
}

Result<void, Error> Ext4Node::rmdir(const char* name) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->m_ext2->rmdir(name);
}

} // namespace fkernel
