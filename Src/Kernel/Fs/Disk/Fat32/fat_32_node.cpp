#include <Kernel/Fs/Disk/Fat32/fat_32_node.h>
#include <Kernel/Fs/Disk/Fat32/fat_32_fs.h>
#include <Kernel/Fs/Disk/Fat32/directory_entry.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

Fat32Node::Fat32Node(fk::RefPtr<Fat32FileSystem> fs, uint32_t cluster, size_t size, bool is_dir,
                     uint32_t dir_sector, uint8_t dir_entry_idx)
    : m_fs(fs), m_first_cluster(cluster), m_size(size), m_is_dir(is_dir),
      m_dir_sector(dir_sector), m_dir_entry_idx(dir_entry_idx) {}

fk::core::Result<size_t, fk::core::Error>
Fat32Node::read(uint64_t offset, size_t size, uint8_t *buffer) {
    if (offset >= m_size) return 0;
    if (offset + size > m_size) size = m_size - offset;

    return m_fs->read_from_cluster_chain(m_first_cluster, offset, size, buffer);
}

fk::core::Result<size_t, fk::core::Error>
Fat32Node::write(uint64_t offset, size_t size, const uint8_t* buffer) {
    if (m_is_dir) return fk::core::Error::IsDirectory;
    auto res = m_fs->write_to_cluster_chain(m_first_cluster, offset, size, buffer, m_size);
    if (!res.is_error() && m_dir_sector != 0)
        (void)m_fs->update_dir_entry_size(m_dir_sector, m_dir_entry_idx, (uint32_t)m_size);
    return res;
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> Fat32Node::lookup(const char* name) {
    if (!m_is_dir) return fk::core::Error::NotADirectory;
    return m_fs->find_in_directory(m_first_cluster, name);
}

fk::core::Result<void, fk::core::Error> Fat32Node::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    if (!m_is_dir) return fk::core::Error::NotADirectory;
    return m_fs->list_directory_from(m_first_cluster, entries);
}

}
