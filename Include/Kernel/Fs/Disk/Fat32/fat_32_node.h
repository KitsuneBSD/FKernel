#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>

namespace fkernel {

class Fat32FileSystem;

class Fat32Node : public Node {
    fk::RefPtr<Fat32FileSystem> m_fs;
    uint32_t m_first_cluster;
    size_t m_size;
    bool m_is_dir;
    uint32_t m_dir_sector{0};   // LBA sector of this file's directory entry
    uint8_t  m_dir_entry_idx{0}; // index of the entry within that sector (0-15)

public:
    Fat32Node(fk::RefPtr<Fat32FileSystem> fs, uint32_t cluster, size_t size, bool is_dir,
              uint32_t dir_sector = 0, uint8_t dir_entry_idx = 0);

    virtual fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t *buffer) override;

    virtual fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t *buffer) override;

    virtual size_t size() const override { return m_size; }
    virtual bool is_directory() const override { return m_is_dir; }

    virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
    virtual fk::core::Result<void, fk::core::Error> list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;

    // Metadata operations (Phase 31 — writable FAT32)
    virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> create_child(const char* name, int mode) override;
    virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> mkdir(const char* name, int mode) override;
    virtual fk::core::Result<void, fk::core::Error> unlink(const char* name) override;
    virtual fk::core::Result<void, fk::core::Error> rmdir(const char* name) override;
    virtual fk::core::Result<void, fk::core::Error> truncate(uint64_t new_size) override;
};

}
