#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Fs/Disk/Ufs/ufs_super.h>
#include <Kernel/Driver/Storage/Interfaces/storage_device.h>

namespace fkernel {

class UfsNode;

class UfsFileSystem : public Node {
    friend class UfsNode;

public:
    fk::RefPtr<StorageDevice> m_device;
    UfsMountInfo              m_info;

    static fk::core::Result<fk::RefPtr<UfsFileSystem>, fk::core::Error>
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

    // Shared helpers
    fk::core::Result<void, fk::core::Error>
    read_inode1(uint32_t ino, Ufs1Dinode& out);
    fk::core::Result<void, fk::core::Error>
    read_inode2(uint32_t ino, Ufs2Dinode& out);
    fk::core::Result<uint64_t, fk::core::Error>
    get_block_frag(uint32_t ino, uint32_t logical_block);
    fk::core::Result<uint64_t, fk::core::Error>
    get_block_frag2(uint64_t ino, uint32_t logical_block);
    fk::core::Result<size_t, fk::core::Error>
    read_from_ino(uint32_t ino, uint64_t offset, size_t size, uint8_t* buf);
    fk::core::Result<void, fk::core::Error>
    list_dir_ino(uint32_t ino, uint64_t dir_size,
                 fk::containers::Vector<DirectoryEntry>& entries);
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup_in_ino(uint32_t ino, uint64_t dir_size, const char* name);

private:
    UfsFileSystem() = default;

    static constexpr uint32_t UFS_ROOT_INO = 2;
};

} // namespace fkernel
