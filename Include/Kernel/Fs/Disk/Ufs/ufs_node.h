#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Fs/Disk/Ufs/ufs_super.h>

namespace fkernel {

class UfsFileSystem;

class UfsNode : public Node {
    fk::RefPtr<UfsFileSystem> m_fs;
    uint32_t m_ino;
    uint64_t m_size;
    uint16_t m_mode;

public:
    UfsNode(fk::RefPtr<UfsFileSystem> fs, uint32_t ino, uint64_t size, uint16_t mode);

    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    size_t size() const override { return static_cast<size_t>(m_size); }
    bool is_directory() const override { return (m_mode & UFS_IFMT) == UFS_IFDIR; }
    bool is_symlink() const override  { return (m_mode & UFS_IFMT) == UFS_IFLNK; }

    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup(const char* name) override;
    fk::core::Result<void, fk::core::Error>
    list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
    fk::core::Result<fk::text::String, fk::core::Error>
    read_link() override;

    uint32_t ino() const { return m_ino; }
};

} // namespace fkernel
