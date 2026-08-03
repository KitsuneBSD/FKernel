#include <Kernel/Fs/Disk/MinixFs/minix_node.h>
#include <Kernel/Fs/Disk/MinixFs/minix_fs.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Text/string.h>

namespace fkernel {

using fk::core::Error;

MinixNode::MinixNode(fk::RefPtr<MinixFileSystem> fs, uint32_t ino,
                     const MinixInode& inode)
    : m_fs(fs), m_ino(ino), m_inode(inode) {}

fk::core::Result<size_t, Error>
MinixNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
    if (is_directory()) return Error::IsDirectory;
    return m_fs->read_from_inode(m_inode, offset, size, buffer);
}

fk::core::Result<size_t, Error>
MinixNode::write(uint64_t offset, size_t size, const uint8_t* buffer) {
    if (is_directory()) return Error::IsDirectory;

    uint32_t zsize          = m_fs->zone_size();
    size_t total            = 0;
    uint64_t cur            = offset;

    while (total < size) {
        uint32_t zone_idx  = static_cast<uint32_t>(cur / zsize);
        uint32_t zone_off  = static_cast<uint32_t>(cur % zsize);
        size_t   chunk     = zsize - zone_off;
        if (chunk > size - total) chunk = size - total;

        uint16_t zone = TRY(m_fs->get_data_zone(m_inode, zone_idx));
        if (zone == 0) {
            // Allocate a new zone; only direct zones are supported for extension.
            if (zone_idx >= MINIX_DIRECT_ZONES) return Error::NotSupported;
            zone = TRY(m_fs->allocate_zone_num());
            m_inode.i_zone[zone_idx] = zone;
        }

        uint8_t zbuf[MINIX_BLOCK_SIZE];
        TRY(m_fs->read_block(zone, zbuf));
        fk::memory::copy(zbuf + zone_off, buffer + total, chunk);
        TRY(m_fs->write_block(zone, zbuf));

        total += chunk;
        cur   += chunk;
    }

    if (cur > m_inode.i_size) {
        m_inode.i_size = static_cast<uint32_t>(cur);
    }
    TRY(m_fs->write_inode(m_ino, m_inode));
    return total;
}

fk::core::Result<fk::RefPtr<Node>, Error>
MinixNode::lookup(const char* name) {
    if (!is_directory()) return Error::NotADirectory;
    return m_fs->lookup_in_inode(m_ino, m_inode, name);
}

fk::core::Result<void, Error>
MinixNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    if (!is_directory()) return Error::NotADirectory;
    return m_fs->list_dir_inode(m_inode, entries);
}

fk::core::Result<fk::RefPtr<Node>, Error>
MinixNode::create_child(const char* name, int mode) {
    if (!is_directory()) return Error::NotADirectory;
    return m_fs->create_in_inode(m_ino, m_inode,
                                  name,
                                  static_cast<uint16_t>(0100000u | (mode & 0777u)));
}

fk::core::Result<fk::RefPtr<Node>, Error>
MinixNode::mkdir(const char* name, int mode) {
    if (!is_directory()) return Error::NotADirectory;
    return m_fs->create_in_inode(m_ino, m_inode,
                                  name,
                                  static_cast<uint16_t>(0040000u | (mode & 0777u)));
}

fk::core::Result<void, Error>
MinixNode::unlink(const char* name) {
    if (!is_directory()) return Error::NotADirectory;
    return m_fs->remove_from_inode(m_ino, m_inode, name, false);
}

fk::core::Result<void, Error>
MinixNode::rmdir(const char* name) {
    if (!is_directory()) return Error::NotADirectory;
    return m_fs->remove_from_inode(m_ino, m_inode, name, true);
}

fk::core::Result<void, Error>
MinixNode::truncate(uint64_t new_size) {
    if (is_directory()) return Error::IsDirectory;
    return m_fs->truncate_inode(m_ino, m_inode, new_size);
}

fk::core::Result<fk::text::String, Error>
MinixNode::read_link() {
    if (!is_symlink()) return Error::NotASymlink;

    uint8_t buf[MINIX_BLOCK_SIZE];
    size_t len = m_inode.i_size < MINIX_BLOCK_SIZE ? m_inode.i_size
                                                    : MINIX_BLOCK_SIZE - 1;
    TRY(m_fs->read_from_inode(m_inode, 0, len, buf));
    buf[len] = '\0';
    return fk::text::String(reinterpret_cast<const char*>(buf));
}

} // namespace fkernel
