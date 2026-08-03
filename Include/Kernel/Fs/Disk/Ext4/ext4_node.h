#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Fs/Disk/Ext2/ext2_super.h>

namespace fkernel {

class Ext4FileSystem;

class Ext4Node : public Node {
    fk::RefPtr<Ext4FileSystem> m_fs;
    uint32_t                   m_ino;
    Ext2Inode                  m_inode;
    bool                       m_is_dir;

public:
    Ext4Node(fk::RefPtr<Ext4FileSystem> fs, uint32_t ino, const Ext2Inode& inode);

    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    size_t size() const override { return m_inode.i_size; }
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
};

} // namespace fkernel
