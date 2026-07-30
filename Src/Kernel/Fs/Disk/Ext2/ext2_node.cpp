#include <Kernel/Fs/Disk/Ext2/ext2_node.h>
#include <Kernel/Fs/Disk/Ext2/ext2_fs.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Text/string.h>
#include <LibFK/Memory/heap_malloc.h>

namespace fkernel {

using fk::core::Error;

Ext2Node::Ext2Node(fk::RefPtr<Ext2FileSystem> fs, uint32_t ino, const Ext2Inode& inode)
    : m_fs(fs), m_ino(ino), m_inode(inode) {}

fk::core::Result<size_t, Error>
Ext2Node::read(uint64_t offset, size_t size, uint8_t* buffer) {
    if (is_directory()) return Error::IsDirectory;
    return m_fs->read_from_inode(m_inode, offset, size, buffer);
}

fk::core::Result<size_t, Error>
Ext2Node::write(uint64_t offset, size_t size, const uint8_t* buffer) {
    if (is_directory()) return Error::IsDirectory;

    size_t total = 0;
    uint64_t cur = offset;

    while (total < size) {
        uint32_t lblock  = static_cast<uint32_t>(cur / m_fs->m_block_size);
        uint32_t blk_off = static_cast<uint32_t>(cur % m_fs->m_block_size);
        size_t   chunk   = m_fs->m_block_size - blk_off;
        if (chunk > size - total) chunk = size - total;

        uint32_t phys = TRY(m_fs->set_data_block(m_inode, m_ino, lblock, true));
        if (phys == 0) return Error::IOError;

        uint8_t* blkbuf = static_cast<uint8_t*>(kmalloc(m_fs->m_block_size));
        if (!blkbuf) return Error::OutOfMemory;
        if (m_fs->read_block(phys, blkbuf).is_error()) { kfree(blkbuf); return Error::IOError; }
        fk::memory::copy(blkbuf + blk_off, buffer + total, chunk);
        m_fs->write_block(phys, blkbuf);
        kfree(blkbuf);

        total += chunk;
        cur   += chunk;
    }

    if (cur > m_inode.i_size) {
        m_inode.i_size = static_cast<uint32_t>(cur);
        m_fs->write_inode(m_ino, m_inode);
    }
    return total;
}

fk::core::Result<fk::RefPtr<Node>, Error>
Ext2Node::lookup(const char* name) {
    if (!is_directory()) return Error::NotADirectory;
    return m_fs->lookup_in_inode(m_inode, name);
}

fk::core::Result<void, Error>
Ext2Node::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    if (!is_directory()) return Error::NotADirectory;
    return m_fs->list_dir_inode(m_inode, entries);
}

fk::core::Result<fk::RefPtr<Node>, Error>
Ext2Node::create_child(const char* name, int mode) {
    if (!is_directory()) return Error::NotADirectory;
    return m_fs->create_in_dir(m_ino, m_inode, name,
                                static_cast<uint16_t>(0100000u | (mode & 0777u)));
}

fk::core::Result<fk::RefPtr<Node>, Error>
Ext2Node::mkdir(const char* name, int mode) {
    if (!is_directory()) return Error::NotADirectory;
    return m_fs->create_in_dir(m_ino, m_inode, name,
                                static_cast<uint16_t>(0040000u | (mode & 0777u)));
}

fk::core::Result<void, Error>
Ext2Node::unlink(const char* name) {
    if (!is_directory()) return Error::NotADirectory;
    return m_fs->remove_from_dir(m_ino, m_inode, name, false);
}

fk::core::Result<void, Error>
Ext2Node::rmdir(const char* name) {
    if (!is_directory()) return Error::NotADirectory;
    return m_fs->remove_from_dir(m_ino, m_inode, name, true);
}

fk::core::Result<void, Error>
Ext2Node::truncate(uint64_t new_size) {
    if (is_directory()) return Error::IsDirectory;
    return m_fs->truncate_inode(m_ino, m_inode, new_size);
}

fk::core::Result<fk::text::String, Error>
Ext2Node::read_link() {
    if (!is_symlink()) return Error::NotASymlink;

    // Short symlink stored inline in i_block[]
    if (m_inode.i_size <= EXT2_SYMLINK_INLINE_MAX && m_inode.i_blocks == 0) {
        char target[EXT2_SYMLINK_INLINE_MAX + 1];
        fk::memory::copy(target, m_inode.i_block, m_inode.i_size);
        target[m_inode.i_size] = '\0';
        return fk::text::String(target);
    }

    size_t len = m_inode.i_size;
    uint8_t* buf = static_cast<uint8_t*>(kmalloc(len + 1));
    if (!buf) return Error::OutOfMemory;
    auto res = m_fs->read_from_inode(m_inode, 0, len, buf);
    if (res.is_error()) { kfree(buf); return res.error(); }
    buf[len] = '\0';
    fk::text::String result(reinterpret_cast<const char*>(buf));
    kfree(buf);
    return result;
}

} // namespace fkernel
