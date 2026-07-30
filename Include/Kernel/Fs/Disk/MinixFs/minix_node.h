#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Fs/Disk/MinixFs/minix_super.h>

namespace fkernel {

class MinixFileSystem;

class MinixNode : public Node {
    fk::RefPtr<MinixFileSystem> m_fs;
    uint32_t  m_ino;
    MinixInode m_inode;

public:
    MinixNode(fk::RefPtr<MinixFileSystem> fs, uint32_t ino, const MinixInode& inode);

    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    size_t size() const override { return m_inode.i_size; }
    bool is_directory() const override { return (m_inode.i_mode & 0170000u) == 0040000u; }
    bool is_symlink() const override  { return (m_inode.i_mode & 0170000u) == 0120000u; }

    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup(const char* name) override;
    fk::core::Result<void, fk::core::Error>
    list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    create_child(const char* name, int mode) override;
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    mkdir(const char* name, int mode) override;
    fk::core::Result<void, fk::core::Error> unlink(const char* name) override;
    fk::core::Result<void, fk::core::Error> rmdir(const char* name) override;
    fk::core::Result<void, fk::core::Error> truncate(uint64_t new_size) override;
    fk::core::Result<fk::text::String, fk::core::Error> read_link() override;

    uint32_t ino() const { return m_ino; }
    const MinixInode& raw_inode() const { return m_inode; }
};

} // namespace fkernel
