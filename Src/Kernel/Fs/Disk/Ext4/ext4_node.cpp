#include <LibFK/Memory/Allocators/heap_malloc.h>
#include <LibFK/Utilities/memory.h>

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

Result<size_t, Error> Ext4Node::write(uint64_t offset, size_t size, const uint8_t* buffer) {
    if (m_is_dir) return Error::IsDirectory;

    // Re-read on-disk i_flags: extent files must not be written through the
    // ext2 indirect-block path (it would corrupt the extent tree).
    Ext2Inode on_disk;
    TRY(m_fs->read_inode_ext4(m_ino, on_disk));
    if ((on_disk.i_flags & EXT2_EXTENTS_FL) != 0) {
        return Error::NotSupported; // extent-tree allocation not implemented
    }

    // Indirect-block layout: reuse the ext2 data-write machinery.
    auto* ext2 = m_fs->m_ext2.ptr();
    size_t total = 0;
    uint64_t cur = offset;

    while (total < size) {
        uint32_t lblock  = static_cast<uint32_t>(cur / ext2->m_block_size);
        uint32_t blk_off = static_cast<uint32_t>(cur % ext2->m_block_size);
        size_t   chunk   = ext2->m_block_size - blk_off;
        if (chunk > size - total) chunk = size - total;

        uint32_t phys = TRY(ext2->set_data_block(m_inode, m_ino, lblock, true));
        if (phys == 0) return Error::IOError;

        uint8_t* blkbuf = static_cast<uint8_t*>(kmalloc(ext2->m_block_size));
        if (!blkbuf) return Error::OutOfMemory;
        if (ext2->read_block(phys, blkbuf).is_error()) { kfree(blkbuf); return Error::IOError; }
        fk::memory::copy(blkbuf + blk_off, buffer + total, chunk);
        ext2->write_block(phys, blkbuf);
        kfree(blkbuf);

        total += chunk;
        cur   += chunk;
    }

    if (cur > m_inode.i_size) {
        m_inode.i_size = static_cast<uint32_t>(cur);
        ext2->write_inode(m_ino, m_inode);
    }
    return total;
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
