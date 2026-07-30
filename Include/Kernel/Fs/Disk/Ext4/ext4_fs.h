#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Fs/Disk/Ext2/ext2_fs.h>
#include <Kernel/Fs/Disk/Ext4/ext4_super.h>
#include <Kernel/Driver/Storage/storage_device.h>

namespace fkernel {

class Ext4Node;

class Ext4FileSystem : public Node {
    friend class Ext4Node;

public:
    // Public fields accessed by Ext4Node
    fk::RefPtr<StorageDevice> m_device;
    Ext2Superblock            m_super;
    uint32_t                  m_block_size;
    uint32_t                  m_inode_size;
    uint32_t                  m_ptrs_per_blk;
    uint32_t                  m_bgdt_block;
    fk::RefPtr<Ext2FileSystem> m_ext2; // for write operations

    static fk::core::Result<fk::RefPtr<Ext4FileSystem>, fk::core::Error>
    create(fk::RefPtr<StorageDevice> device);

    // Root Node interface
    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    size_t size() const override { return 0; }
    bool is_directory() const override { return true; }
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
    fk::core::Result<void, fk::core::Error>
    list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    create_child(const char* name, int mode) override;
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    mkdir(const char* name, int mode) override;
    fk::core::Result<void, fk::core::Error> unlink(const char* name) override;
    fk::core::Result<void, fk::core::Error> rmdir(const char* name) override;

    // Extent-aware block I/O helpers
    fk::core::Result<void, fk::core::Error> read_inode_ext4(uint32_t ino, Ext2Inode& out);
    fk::core::Result<uint64_t, fk::core::Error>
    get_extent_block(const Ext2Inode& inode, uint32_t logical);
    fk::core::Result<uint64_t, fk::core::Error>
    get_data_block_ext4(const Ext2Inode& inode, uint32_t logical);
    fk::core::Result<size_t, fk::core::Error>
    read_from_inode_ext4(const Ext2Inode& inode, uint64_t offset, size_t size, uint8_t* buf);
    fk::core::Result<void, fk::core::Error>
    list_dir_inode_ext4(const Ext2Inode& dir, fk::containers::Vector<DirectoryEntry>& entries);
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup_in_inode_ext4(const Ext2Inode& dir, uint32_t dir_ino, const char* name);

    fk::core::Result<void, fk::core::Error> read_block(uint32_t block, uint8_t* buf);

private:
    Ext4FileSystem(fk::RefPtr<StorageDevice> device, fk::RefPtr<Ext2FileSystem> ext2)
        : m_device(device), m_ext2(fk::types::move(ext2)) {}

    fk::RefPtr<Node> make_node_ext4(uint32_t ino, const Ext2Inode& inode);
};

} // namespace fkernel
