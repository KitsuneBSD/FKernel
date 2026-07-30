#pragma once

#include <Kernel/Fs/Disk/HfsPlus/hfsplus_vh.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_btree.h>
#include <Kernel/Driver/Storage/storage_device.h>
#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Core/result.h>
#include <LibFK/Memory/ref_ptr.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class HFSPlusNode;

class HFSPlusFileSystem : public Node {
    friend class HFSPlusNode;

public:
    fk::RefPtr<StorageDevice> m_device;
    uint32_t  m_block_size{0};
    uint32_t  m_first_sector{0};  // byte offset 0 of the partition in device sectors
    BTreeFile m_catalog;
    BTreeFile m_extents;
    bool      m_case_sensitive{false};

    static fk::core::Result<fk::RefPtr<HFSPlusFileSystem>, fk::core::Error>
    create(fk::RefPtr<StorageDevice> device);

    // Root Node interface
    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t, size_t, uint8_t*) override { return fk::core::Error::IsDirectory; }
    fk::core::Result<size_t, fk::core::Error>
    write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::PermissionDenied; }
    size_t size() const override { return 0; }
    bool is_directory() const override { return true; }
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup(const char* name) override;
    fk::core::Result<void, fk::core::Error>
    list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;

private:
    HFSPlusFileSystem() = default;

    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup_in(uint32_t parent_cnid, const char* name);

    fk::core::Result<void, fk::core::Error>
    list_dir_in(uint32_t parent_cnid, fk::containers::Vector<DirectoryEntry>& entries);

    // Read a BTreeFile backed by a fork whose extents start at m_first_sector
    fk::core::Result<void, fk::core::Error>
    open_btree(BTreeFile& out, const HFSPlusForkData& fork);
};

} // namespace fkernel
