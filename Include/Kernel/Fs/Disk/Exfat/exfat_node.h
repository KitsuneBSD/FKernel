#pragma once

#include <Kernel/Fs/Vfs/node.h>

namespace fkernel {

class ExfatFileSystem;

class ExfatNode : public Node {
    fk::RefPtr<ExfatFileSystem> m_fs;
    uint32_t m_first_cluster;
    uint64_t m_data_length;
    bool     m_is_dir;
    bool     m_no_fat_chain;
    uint32_t m_parent_cluster;   // first cluster of parent directory
    uint32_t m_dir_entry_offset; // byte offset in parent's cluster chain of our File entry

public:
    ExfatNode(fk::RefPtr<ExfatFileSystem> fs, uint32_t first_cluster, uint64_t data_length,
              bool is_dir, bool no_fat_chain,
              uint32_t parent_cluster, uint32_t dir_entry_offset);

    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    size_t size() const override { return static_cast<size_t>(m_data_length); }
    bool is_directory() const override { return m_is_dir; }

    fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
    fk::core::Result<void, fk::core::Error>
    list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    create_child(const char* name, int mode) override;
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    mkdir(const char* name, int mode) override;
    fk::core::Result<void, fk::core::Error> unlink(const char* name) override;
    fk::core::Result<void, fk::core::Error> rmdir(const char* name) override;
    fk::core::Result<void, fk::core::Error> truncate(uint64_t new_size) override;
};

} // namespace fkernel
