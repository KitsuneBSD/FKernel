#include <Kernel/Fs/Disk/Iso9660/iso9660_node.h>
#include <Kernel/Fs/Disk/Iso9660/iso9660_fs.h>

namespace fkernel {

using fk::core::Error;

Iso9660Node::Iso9660Node(fk::RefPtr<Iso9660FileSystem> fs, uint32_t lba,
                          uint32_t data_size, bool is_dir, bool is_symlink,
                          fk::text::String target)
    : m_fs(fk::types::move(fs))
    , m_lba(lba)
    , m_data_size(data_size)
    , m_is_dir(is_dir)
    , m_is_symlink(is_symlink)
    , m_symlink_target(fk::types::move(target))
{}

fk::core::Result<size_t, Error>
Iso9660Node::read(uint64_t offset, size_t size, uint8_t* buffer) {
    if (m_is_dir) return Error::IsDirectory;
    if (offset >= m_data_size) return static_cast<size_t>(0);
    size_t avail = static_cast<size_t>(m_data_size - offset);
    if (size > avail) size = avail;
    return m_fs->read_bytes(m_lba, offset, size, buffer);
}

fk::core::Result<size_t, Error>
Iso9660Node::write(uint64_t /*offset*/, size_t /*size*/, const uint8_t* /*buffer*/) {
    return Error::ReadOnly;
}

fk::core::Result<fk::RefPtr<Node>, Error>
Iso9660Node::lookup(const char* name) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->lookup_lba(m_lba, m_data_size, name);
}

fk::core::Result<void, Error>
Iso9660Node::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->list_dir_lba(m_lba, m_data_size, entries);
}

fk::core::Result<fk::text::String, Error>
Iso9660Node::read_link() {
    if (!m_is_symlink) return Error::NotASymlink;
    return m_symlink_target;
}

} // namespace fkernel
