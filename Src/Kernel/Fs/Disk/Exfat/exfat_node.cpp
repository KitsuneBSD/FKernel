#include <Kernel/Fs/Disk/Exfat/exfat_node.h>
#include <Kernel/Fs/Disk/Exfat/exfat_fs.h>

namespace fkernel {

using fk::core::Error;

ExfatNode::ExfatNode(fk::RefPtr<ExfatFileSystem> fs, uint32_t first_cluster,
                      uint64_t data_length, bool is_dir, bool no_fat_chain,
                      uint32_t parent_cluster, uint32_t dir_entry_offset)
    : m_fs(fk::types::move(fs))
    , m_first_cluster(first_cluster)
    , m_data_length(data_length)
    , m_is_dir(is_dir)
    , m_no_fat_chain(no_fat_chain)
    , m_parent_cluster(parent_cluster)
    , m_dir_entry_offset(dir_entry_offset)
{}

fk::core::Result<size_t, Error>
ExfatNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
    if (m_is_dir) return Error::IsDirectory;
    return m_fs->read_chain(m_first_cluster, m_no_fat_chain,
                            m_data_length, offset, size, buffer);
}

fk::core::Result<size_t, Error>
ExfatNode::write(uint64_t offset, size_t size, const uint8_t* buffer) {
    if (m_is_dir) return Error::IsDirectory;
    auto res = m_fs->write_chain(m_first_cluster, m_no_fat_chain,
                                  m_data_length, offset, size, buffer);
    if (res.is_error()) return res.error();
    if (m_data_length > 0)
        m_fs->update_stream_ext(m_parent_cluster, m_dir_entry_offset, m_data_length);
    return res;
}

fk::core::Result<fk::RefPtr<Node>, Error>
ExfatNode::lookup(const char* name) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->lookup_cluster(m_first_cluster, name);
}

fk::core::Result<void, Error>
ExfatNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->list_dir_cluster(m_first_cluster, entries);
}

fk::core::Result<fk::RefPtr<Node>, Error>
ExfatNode::create_child(const char* name, int /*mode*/) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->create_entry(m_first_cluster, name, false);
}

fk::core::Result<fk::RefPtr<Node>, Error>
ExfatNode::mkdir(const char* name, int /*mode*/) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->create_entry(m_first_cluster, name, true);
}

fk::core::Result<void, Error>
ExfatNode::unlink(const char* name) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->delete_entry(m_first_cluster, name);
}

fk::core::Result<void, Error>
ExfatNode::rmdir(const char* name) {
    if (!m_is_dir) return Error::NotADirectory;
    return m_fs->delete_entry(m_first_cluster, name);
}

fk::core::Result<void, Error>
ExfatNode::truncate(uint64_t new_size) {
    if (m_is_dir) return Error::IsDirectory;
    m_data_length = new_size;
    return m_fs->update_stream_ext(m_parent_cluster, m_dir_entry_offset, new_size);
}

} // namespace fkernel
