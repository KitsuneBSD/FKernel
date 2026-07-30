#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_vh.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_catalog.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_fs.h>
#include <LibFK/Core/result.h>
#include <LibFK/Memory/ref_ptr.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class HFSPlusNode final : public Node {
public:
    static fk::RefPtr<HFSPlusNode> create_file(
        fk::RefPtr<HFSPlusFileSystem> fs,
        uint32_t cnid,
        const HFSPlusCatalogFile& rec);

    static fk::RefPtr<HFSPlusNode> create_dir(
        fk::RefPtr<HFSPlusFileSystem> fs,
        uint32_t cnid);

    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buf) override;

    fk::core::Result<size_t, fk::core::Error>
    write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::PermissionDenied; }

    size_t size() const override { return m_size; }
    bool is_directory() const override { return m_is_dir; }

    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup(const char* name) override;

    fk::core::Result<void, fk::core::Error>
    list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;

    bool is_symlink() const override { return m_is_symlink; }
    fk::core::Result<fk::text::String, fk::core::Error> read_link() override;

private:
    HFSPlusNode() = default;

    fk::RefPtr<HFSPlusFileSystem> m_fs;
    uint32_t          m_cnid{0};
    HFSPlusForkData   m_data_fork{};
    uint64_t          m_size{0};
    bool              m_is_dir{false};
    bool              m_is_symlink{false};
    uint16_t          m_mode{0};
};

} // namespace fkernel
