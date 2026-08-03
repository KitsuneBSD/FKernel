#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>

namespace fkernel {

class Fat12FileSystem;

class Fat12Node : public Node {
    fk::RefPtr<Fat12FileSystem> m_fs;
    uint32_t m_first_cluster;
    size_t m_size;
    bool m_is_dir;

public:
    Fat12Node(fk::RefPtr<Fat12FileSystem> fs, uint32_t cluster, size_t size, bool is_dir);

    virtual fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t *buffer) override;

    virtual fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t *buffer) override;

    virtual size_t size() const override { return m_size; }
    virtual bool is_directory() const override { return m_is_dir; }
};

}
