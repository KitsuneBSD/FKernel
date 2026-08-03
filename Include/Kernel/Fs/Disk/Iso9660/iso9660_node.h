#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Text/string.h>

namespace fkernel {

class Iso9660FileSystem;

class Iso9660Node : public Node {
    fk::RefPtr<Iso9660FileSystem> m_fs;
    uint32_t m_lba;
    uint32_t m_data_size;
    bool     m_is_dir;
    bool     m_is_symlink;
    fk::text::String m_symlink_target;

public:
    Iso9660Node(fk::RefPtr<Iso9660FileSystem> fs, uint32_t lba, uint32_t data_size,
                bool is_dir, bool is_symlink, fk::text::String target);

    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    size_t size() const override { return m_data_size; }
    bool is_directory() const override { return m_is_dir; }
    bool is_symlink() const override  { return m_is_symlink; }

    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup(const char* name) override;
    fk::core::Result<void, fk::core::Error>
    list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
    fk::core::Result<fk::text::String, fk::core::Error>
    read_link() override;

    // Read-only: write operations always fail
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    create_child(const char* name, int mode) override { (void)name;(void)mode; return fk::core::Error::ReadOnly; }
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    mkdir(const char* name, int mode) override { (void)name;(void)mode; return fk::core::Error::ReadOnly; }
    fk::core::Result<void, fk::core::Error>
    unlink(const char* name) override { (void)name; return fk::core::Error::ReadOnly; }
    fk::core::Result<void, fk::core::Error>
    rmdir(const char* name) override { (void)name; return fk::core::Error::ReadOnly; }
    fk::core::Result<void, fk::core::Error>
    truncate(uint64_t size) override { (void)size; return fk::core::Error::ReadOnly; }
};

} // namespace fkernel
