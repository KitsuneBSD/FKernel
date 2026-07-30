#include <Kernel/Fs/Disk/HfsPlus/hfsplus_node.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_btree.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_unicode.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_extents.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

fk::RefPtr<HFSPlusNode> HFSPlusNode::create_file(
    fk::RefPtr<HFSPlusFileSystem> fs,
    uint32_t cnid,
    const HFSPlusCatalogFile& rec)
{
    auto* n = new HFSPlusNode();
    if (!n) return {};
    n->m_fs       = fs;
    n->m_cnid     = cnid;
    n->m_is_dir   = false;
    n->m_mode     = hfs_be16(rec.fileMode);
    // S_IFLNK = 0xA000
    n->m_is_symlink = ((n->m_mode & 0xF000) == 0xA000);
    fk::memory::copy(&n->m_data_fork, &rec.dataFork, sizeof(HFSPlusForkData));
    n->m_size = hfs_be64(rec.dataFork.logicalSize);
    return fk::RefPtr<HFSPlusNode>(n);
}

fk::RefPtr<HFSPlusNode> HFSPlusNode::create_dir(
    fk::RefPtr<HFSPlusFileSystem> fs,
    uint32_t cnid)
{
    auto* n = new HFSPlusNode();
    if (!n) return {};
    n->m_fs     = fs;
    n->m_cnid   = cnid;
    n->m_is_dir = true;
    return fk::RefPtr<HFSPlusNode>(n);
}

fk::core::Result<size_t, fk::core::Error>
HFSPlusNode::read(uint64_t offset, size_t size, uint8_t* buf)
{
    if (m_is_dir) return fk::core::Error::IsDirectory;

    HFSPlusForkReader reader;
    reader.set(m_fs->m_device, &m_fs->m_extents,
               m_data_fork,
               m_fs->m_block_size,
               m_fs->m_first_sector,
               m_cnid, 0 /* data fork */);
    return reader.read(offset, size, buf);
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
HFSPlusNode::lookup(const char* name)
{
    if (!m_is_dir) return fk::core::Error::NotADirectory;
    return m_fs->lookup_in(m_cnid, name);
}

struct HfsListCtx {
    fk::containers::Vector<DirectoryEntry>* entries;
};

static void hfs_list_cb(void* ctx, const char* name, const CatalogRecord& rec) {
    auto* lc = static_cast<HfsListCtx*>(ctx);
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
HFSPlusNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries)
{
    if (!m_is_dir) return fk::core::Error::NotADirectory;
    HfsListCtx ctx{&entries};
    return btree_catalog_list(m_fs->m_catalog, m_cnid, hfs_list_cb, &ctx);
}

fk::core::Result<fk::text::String, fk::core::Error>
HFSPlusNode::read_link()
{
    if (!m_is_symlink) return fk::core::Error::NotImplemented;

    // Symlink target is stored in the data fork as a plain UTF-8 string
    fk::containers::Vector<uint8_t> buf;
    buf.resize((size_t)m_size + 1);
    auto res = read(0, (size_t)m_size, buf.begin());
    if (res.is_error()) return res.error();
    buf[res.value()] = '\0';
    return fk::text::String(reinterpret_cast<const char*>(buf.begin()));
}

} // namespace fkernel
