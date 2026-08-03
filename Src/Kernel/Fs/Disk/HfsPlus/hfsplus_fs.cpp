#include <Kernel/Fs/Disk/HfsPlus/hfsplus_fs.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_node.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_unicode.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_extents.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Memory/Allocators/new.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/Generic/byte_order.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<HFSPlusFileSystem>, fk::core::Error>
HFSPlusFileSystem::create(fk::RefPtr<StorageDevice> device)
{
    // Volume Header is always at byte offset 1024 from partition start
    uint32_t sector_size = device->sector_size().value();
    uint64_t vh_sector   = 1024 / sector_size;
    uint32_t vh_off_in   = 1024 % sector_size;

    fk::containers::Vector<uint8_t> buf;
    buf.resize(sector_size);
    if (device->read_sectors(vh_sector, 1, buf.begin()).is_error())
        return fk::core::Error::IOError;

    if (vh_off_in + sizeof(HFSPlusVolumeHeader) > sector_size)
        return fk::core::Error::InvalidData;

    HFSPlusVolumeHeader vh;
    fk::memory::copy(&vh, buf.begin() + vh_off_in, sizeof(HFSPlusVolumeHeader));

    uint16_t sig = fk::algorithms::swap16(vh.signature);
    if (sig != kHFSPlusSig && sig != kHFSXSig)
        return fk::core::Error::InvalidData;

    uint32_t block_size = fk::algorithms::swap32(vh.blockSize);
    if (block_size == 0 || (block_size & (block_size - 1)) != 0)
        return fk::core::Error::InvalidData;

    auto* fs = new HFSPlusFileSystem();
    if (!fs) return fk::core::Error::OutOfMemory;

    fs->m_device         = device;
    fs->m_block_size     = block_size;
    fs->m_first_sector   = 0; // entire device is the partition
    fs->m_case_sensitive = (sig == kHFSXSig &&
                            vh.lastMountedVersion == fk::algorithms::swap32(0x33363063)); // "360c" HFSX

    auto cat_res = fs->open_btree(fs->m_catalog, vh.catalogFile);
    if (cat_res.is_error()) { delete fs; return cat_res.error(); }

    auto ext_res = fs->open_btree(fs->m_extents, vh.extentsFile);
    if (ext_res.is_error()) { delete fs; return ext_res.error(); }

    fk::algorithms::klog("HFS+", "Mounted HFS+%s blockSize=%u",
                         (sig == kHFSXSig) ? "X" : "", block_size);

    return fk::RefPtr<HFSPlusFileSystem>(fs);
}

fk::core::Result<void, fk::core::Error>
HFSPlusFileSystem::open_btree(BTreeFile& out, const HFSPlusForkData& fork)
{
    return out.open(m_device, fork, m_block_size, m_first_sector);
}

// ── Root directory VFS interface ─────────────────────────────────────────────

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
HFSPlusFileSystem::lookup(const char* name)
{
    return lookup_in(kHFSRootFolderID, name);
}

fk::core::Result<void, fk::core::Error>
HFSPlusFileSystem::list_dir(fk::containers::Vector<DirectoryEntry>& entries)
{
    return list_dir_in(kHFSRootFolderID, entries);
}

// ── Internal helpers ─────────────────────────────────────────────────────────

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
HFSPlusFileSystem::lookup_in(uint32_t parent_cnid, const char* name)
{
    HFSUniStr255 uname;
    if (!hfsplus_utf8_to_unicode(name, uname))
        return fk::core::Error::InvalidParameter;

    auto rec_res = btree_catalog_lookup(m_catalog, parent_cnid, uname);
    if (rec_res.is_error()) return rec_res.error();

    const CatalogRecord& cr = rec_res.value();

    if (cr.type == kHFSPlusFolderRecord) {
        uint32_t cnid = fk::algorithms::swap32(cr.folder.folderID);
        fk::RefPtr<HFSPlusFileSystem> self(this);
        auto dir_node = HFSPlusNode::create_dir(self, cnid);
        if (!dir_node) return fk::core::Error::OutOfMemory;
        return fk::RefPtr<Node>(dir_node);
    }

    if (cr.type == kHFSPlusFileRecord) {
        uint32_t cnid = fk::algorithms::swap32(cr.file.fileID);
        fk::RefPtr<HFSPlusFileSystem> self(this);
        return fk::RefPtr<Node>(HFSPlusNode::create_file(self, cnid, cr.file));
    }

    return fk::core::Error::NotFound;
}

struct ListCtx {
    fk::containers::Vector<DirectoryEntry>* entries;
};

static void list_cb(void* ctx, const char* name, const CatalogRecord& rec) {
    auto* lc = static_cast<ListCtx*>(ctx);
    DirectoryEntry de;
    size_t len = 0;
    while (name[len] && len < sizeof(de.name) - 1) {
        de.name[len] = name[len];
        ++len;
    }
    de.name[len] = '\0';
    de.type = (rec.type == kHFSPlusFolderRecord) ? 1u
                                                  : 0u;
    lc->entries->push_back(de);
}

fk::core::Result<void, fk::core::Error>
HFSPlusFileSystem::list_dir_in(uint32_t parent_cnid,
                                fk::containers::Vector<DirectoryEntry>& entries)
{
    ListCtx ctx{&entries};
    return btree_catalog_list(m_catalog, parent_cnid, list_cb, &ctx);
}

} // namespace fkernel
