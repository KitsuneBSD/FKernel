#include <Kernel/Fs/Fat12/fat_12_node.h>
#include <Kernel/Fs/Fat12/fat_12_fs.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

Fat12Node::Fat12Node(fk::RefPtr<Fat12FileSystem> fs, uint32_t cluster, size_t size, bool is_dir)
    : m_fs(fs), m_first_cluster(cluster), m_size(size), m_is_dir(is_dir) {}

fk::core::Result<size_t, fk::core::Error>
Fat12Node::read(uint64_t offset, size_t size, uint8_t *buffer) {
    if (offset >= m_size) return 0;
    if (offset + size > m_size) size = m_size - offset;

    return m_fs->read_from_cluster_chain(m_first_cluster, offset, size, buffer);
}

fk::core::Result<size_t, fk::core::Error>
Fat12Node::write(uint64_t offset, size_t size, const uint8_t *buffer) {
    // TODO: Implement cluster allocation if file grows
    return m_fs->write_to_cluster_chain(m_first_cluster, offset, size, buffer);
}

}
