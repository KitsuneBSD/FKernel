#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Fs/Disk/MinixFs/minix_super.h>
#include <Kernel/Driver/Storage/Interfaces/storage_device.h>

namespace fkernel {

class MinixNode;

class MinixFileSystem : public Node {
    friend class MinixNode;

    fk::RefPtr<StorageDevice> m_device;
    MinixSuperBlock m_super;
    uint16_t m_name_len;        // 14 (standard) or 30 (extended)
    uint32_t m_inode_table_blk; // block index of first inode block

public:
    static fk::core::Result<fk::RefPtr<MinixFileSystem>, fk::core::Error>
    create(fk::RefPtr<StorageDevice> device);

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

    // Shared helpers used by MinixNode
    fk::core::Result<void, fk::core::Error>
    read_inode(uint32_t ino, MinixInode& out);
    fk::core::Result<void, fk::core::Error>
    write_inode(uint32_t ino, const MinixInode& inode);
    fk::core::Result<uint16_t, fk::core::Error>
    get_data_zone(const MinixInode& inode, uint32_t zone_index);
    fk::core::Result<size_t, fk::core::Error>
    read_from_inode(const MinixInode& inode, uint64_t offset, size_t size, uint8_t* buf);
    fk::core::Result<void, fk::core::Error>
    list_dir_inode(const MinixInode& inode, fk::containers::Vector<DirectoryEntry>& entries);
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup_in_inode(uint32_t dir_ino, const MinixInode& dir_inode, const char* name);
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    create_in_inode(uint32_t dir_ino, MinixInode& dir_inode, const char* name, uint16_t mode);
    fk::core::Result<void, fk::core::Error>
    remove_from_inode(uint32_t dir_ino, MinixInode& dir_inode, const char* name, bool check_empty);
    fk::core::Result<void, fk::core::Error>
    truncate_inode(uint32_t ino, MinixInode& inode, uint64_t new_size);

    // Bitmap allocation
    fk::core::Result<uint32_t, fk::core::Error> allocate_inode_num();
    fk::core::Result<uint16_t, fk::core::Error> allocate_zone_num();
    fk::core::Result<void, fk::core::Error> free_inode_num(uint32_t ino);
    fk::core::Result<void, fk::core::Error> free_zone_num(uint16_t zone);

    fk::core::Result<void, fk::core::Error>
    read_block(uint32_t block, uint8_t* buf);
    fk::core::Result<void, fk::core::Error>
    write_block(uint32_t block, const uint8_t* buf);
    uint32_t zone_size() const { return MINIX_BLOCK_SIZE << m_super.s_log_zone_size; }
    uint32_t dir_entry_size() const { return m_name_len == 30 ? 32u : 16u; }

private:
    MinixFileSystem(fk::RefPtr<StorageDevice> device) : m_device(device) {}
};

} // namespace fkernel
