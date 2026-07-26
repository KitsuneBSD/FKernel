#include <Kernel/Fs/Disk/Fat16/fat_16_node.h>
#include <Kernel/Fs/Disk/Fat16/fat_16_fs.h>

namespace fkernel {

Fat16Node::Fat16Node(fk::RefPtr<Fat16FileSystem> fs, uint32_t cluster, size_t size, bool is_dir)
    : m_fs(fs), m_first_cluster(cluster), m_size(size), m_is_dir(is_dir) {}

fk::core::Result<size_t, fk::core::Error>
Fat16Node::read(uint64_t offset, size_t size, uint8_t* buffer) {
    if (offset >= m_size) return static_cast<size_t>(0);
    if (offset + size > m_size) size = m_size - static_cast<size_t>(offset);
    return m_fs->read_from_cluster_chain(m_first_cluster, offset, size, buffer);
}

fk::core::Result<size_t, fk::core::Error>
Fat16Node::write(uint64_t, size_t, const uint8_t*) {
    return fk::core::Error::NotImplemented;
}

}
