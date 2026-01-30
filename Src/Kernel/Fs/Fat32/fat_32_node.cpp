#include <Kernel/Fs/Fat32/fat_32_node.h>
#include <Kernel/Fs/Fat32/fat_32_fs.h>
#include <Kernel/Fs/Fat32/directory_entry.h>
#include <LibC/string.h>

namespace fkernel {

Fat32Node::Fat32Node(fk::RefPtr<Fat32FileSystem> fs, uint32_t cluster, size_t size, bool is_dir)
    : m_fs(fs), m_first_cluster(cluster), m_size(size), m_is_dir(is_dir) {}

fk::core::Result<size_t, fk::core::Error>
Fat32Node::read(uint64_t offset, size_t size, uint8_t *buffer) {
    if (offset >= m_size) return 0;
    if (offset + size > m_size) size = m_size - offset;

    return m_fs->read_from_cluster_chain(m_first_cluster, offset, size, buffer);
}

fk::core::Result<size_t, fk::core::Error>
Fat32Node::write(uint64_t, size_t, const uint8_t*) {
    return fk::core::Error::NotImplemented;
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> Fat32Node::lookup([[maybe_unused]] const char* name) {
    if (!m_is_dir) return fk::core::Error::NotADirectory;
    
    fk::containers::Vector<DirectoryEntry> entries;
    list_dir(entries);
    
    // This is a naive lookup by iterating list_dir.
    // In a real FS we'd search the directory clusters directly.
    // But since list_dir is implemented in FS, we can use it.
    // Wait, list_dir in Fat32FileSystem only handles root.
    // We need a proper directory iterator.
    
    return fk::core::Error::NotFound;
}

fk::core::Result<void, fk::core::Error> Fat32Node::list_dir([[maybe_unused]] fk::containers::Vector<DirectoryEntry>& entries) {
    if (!m_is_dir) return fk::core::Error::NotADirectory;
    // TODO: Implement directory listing for non-root directories
    return fk::core::Error::NotImplemented;
}

}
