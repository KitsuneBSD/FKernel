#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Memory/retain_ptr.h>

namespace fkernel {

class Fat32FileSystem;

class Fat32Node : public Node {
    fk::RefPtr<Fat32FileSystem> m_fs;
    uint32_t m_first_cluster;
    size_t m_size;
    bool m_is_dir;

public:
    Fat32Node(fk::RefPtr<Fat32FileSystem> fs, uint32_t cluster, size_t size, bool is_dir);

    virtual fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t *buffer) override;

    virtual fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t *buffer) override;

    virtual size_t size() const override { return m_size; }
    virtual bool is_directory() const override { return m_is_dir; }

    virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
    virtual fk::core::Result<void, fk::core::Error> list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
};

}
