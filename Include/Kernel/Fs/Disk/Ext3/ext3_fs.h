#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Fs/Disk/Ext2/ext2_fs.h>
#include <Kernel/Driver/Storage/storage_device.h>

namespace fkernel {

// Ext3FileSystem wraps Ext2FileSystem, adding JBD journal recovery on mount.
// All read/write operations are delegated to the contained Ext2FileSystem.
// Write-path journaling is not yet implemented (data=writeback equivalent).
class Ext3FileSystem : public Node {
    fk::RefPtr<Ext2FileSystem> m_ext2;

public:
    explicit Ext3FileSystem(fk::RefPtr<Ext2FileSystem> ext2)
        : m_ext2(fk::types::move(ext2)) {}

    static fk::core::Result<fk::RefPtr<Ext3FileSystem>, fk::core::Error>
    create(fk::RefPtr<StorageDevice> device);

    // Delegation to ext2 for all Node operations
    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buffer) override
    { return m_ext2->read(offset, size, buffer); }

    fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buffer) override
    { return m_ext2->write(offset, size, buffer); }

    size_t size() const override { return m_ext2->size(); }
    bool is_directory() const override { return true; }

    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup(const char* name) override { return m_ext2->lookup(name); }

    fk::core::Result<void, fk::core::Error>
    list_dir(fk::containers::Vector<DirectoryEntry>& entries) override
    { return m_ext2->list_dir(entries); }

    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    create_child(const char* name, int mode) override
    { return m_ext2->create_child(name, mode); }

    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    mkdir(const char* name, int mode) override
    { return m_ext2->mkdir(name, mode); }

    fk::core::Result<void, fk::core::Error>
    unlink(const char* name) override { return m_ext2->unlink(name); }

    fk::core::Result<void, fk::core::Error>
    rmdir(const char* name) override { return m_ext2->rmdir(name); }
};

} // namespace fkernel
