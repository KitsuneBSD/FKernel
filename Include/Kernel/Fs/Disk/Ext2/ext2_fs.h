#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Fs/Disk/Ext2/ext2_super.h>
#include <Kernel/Driver/Storage/storage_device.h>

namespace fkernel {

class Ext2Node;

class Ext2FileSystem : public Node {
    friend class Ext2Node;

public:
    fk::RefPtr<StorageDevice> m_device;
    Ext2Superblock m_super;
    uint32_t m_block_size;   // 1024 << s_log_block_size
    uint32_t m_inode_size;   // 128 or s_inode_size for rev1
    uint32_t m_ptrs_per_blk; // block_size / 4
    uint32_t m_bgdt_block;   // block number of Block Group Descriptor Table

    static fk::core::Result<fk::RefPtr<Ext2FileSystem>, fk::core::Error>
    create(fk::RefPtr<StorageDevice> device);

    // Root directory Node interface
    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    size_t size() const override { return 0; }
    bool is_directory() const override { return true; }
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

    // Shared helpers used by Ext2Node
    fk::core::Result<void, fk::core::Error>
    read_inode(uint32_t ino, Ext2Inode& out);
    fk::core::Result<void, fk::core::Error>
    write_inode(uint32_t ino, const Ext2Inode& inode);
    fk::core::Result<uint32_t, fk::core::Error>
    get_data_block(const Ext2Inode& inode, uint32_t logical_block);
    fk::core::Result<uint32_t, fk::core::Error>
    set_data_block(Ext2Inode& inode, uint32_t ino, uint32_t logical_block, bool alloc);
    fk::core::Result<size_t, fk::core::Error>
    read_from_inode(const Ext2Inode& inode, uint64_t offset, size_t size, uint8_t* buf);
    fk::core::Result<void, fk::core::Error>
    list_dir_inode(const Ext2Inode& dir, fk::containers::Vector<DirectoryEntry>& entries);
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup_in_inode(const Ext2Inode& dir, const char* name);
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    create_in_dir(uint32_t dir_ino, Ext2Inode& dir, const char* name, uint16_t mode);
    fk::core::Result<void, fk::core::Error>
    remove_from_dir(uint32_t dir_ino, Ext2Inode& dir, const char* name, bool check_empty);
    fk::core::Result<void, fk::core::Error>
    truncate_inode(uint32_t ino, Ext2Inode& inode, uint64_t new_size);

    // Block/inode allocation
    fk::core::Result<uint32_t, fk::core::Error> alloc_block(uint32_t prefer_bg);
    fk::core::Result<uint32_t, fk::core::Error> alloc_inode(bool is_dir);
    fk::core::Result<void, fk::core::Error> free_block(uint32_t block);
    fk::core::Result<void, fk::core::Error> free_inode(uint32_t ino, bool is_dir);

    fk::core::Result<void, fk::core::Error>
    read_block(uint32_t block, uint8_t* buf);
    fk::core::Result<void, fk::core::Error>
    write_block(uint32_t block, const uint8_t* buf);

private:
    Ext2FileSystem(fk::RefPtr<StorageDevice> device) : m_device(device) {}

    fk::core::Result<void, fk::core::Error>
    read_bg_desc(uint32_t bg, Ext2BlockGroupDesc& out);
    fk::core::Result<void, fk::core::Error>
    write_bg_desc(uint32_t bg, const Ext2BlockGroupDesc& desc);
    uint32_t bg_of_inode(uint32_t ino) const;
    uint32_t local_idx_of_inode(uint32_t ino) const;
};

} // namespace fkernel
