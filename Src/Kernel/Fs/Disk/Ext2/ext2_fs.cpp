#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Algorithms/Generic/bitmap.h>
#include <LibFK/Algorithms/Generic/scatter_io.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>

#include <Kernel/Fs/Disk/Ext2/ext2_fs.h>
#include <Kernel/Fs/Disk/Ext2/ext2_node.h>

namespace fkernel {

using fk::core::Error;

// ---- Block I/O ---------------------------------------------------------------

fk::core::Result<void, Error>
Ext2FileSystem::read_block(uint32_t block, uint8_t* buf) {
    auto res = m_device->read(static_cast<uint64_t>(block) * m_block_size,
                              m_block_size, buf);
    if (res.is_error()) return Error::IOError;
    return {};
}

fk::core::Result<void, Error>
Ext2FileSystem::write_block(uint32_t block, const uint8_t* buf) {
    auto res = m_device->write(static_cast<uint64_t>(block) * m_block_size,
                               m_block_size, buf);
    if (res.is_error()) return Error::IOError;
    return {};
}

// ---- create() factory --------------------------------------------------------

fk::core::Result<fk::RefPtr<Ext2FileSystem>, Error>
Ext2FileSystem::create(fk::RefPtr<StorageDevice> device) {
    Ext2Superblock sb;
    if (device->read(1024, sizeof(sb), reinterpret_cast<uint8_t*>(&sb)).is_error())
        return Error::IOError;

    if (sb.s_magic != EXT2_SUPER_MAGIC)
        return Error::InvalidData;

    // Reject incompat features we can't handle (journal, extents, etc.)
    if (sb.s_rev_level >= 1 &&
        (sb.s_feature_incompat & (EXT2_INCOMPAT_RECOVER | EXT2_INCOMPAT_JOURNAL_DEV)))
        return Error::InvalidData;

    auto fs = fk::adopt_ref(new Ext2FileSystem(device));
    if (!fs) return Error::OutOfMemory;

    fs->m_super       = sb;
    fs->m_block_size  = 1024u << sb.s_log_block_size;
    fs->m_inode_size  = (sb.s_rev_level >= 1 && sb.s_inode_size >= 128)
                        ? sb.s_inode_size : 128u;
    fs->m_ptrs_per_blk = fs->m_block_size / 4;
    fs->m_bgdt_block  = sb.s_first_data_block + 1;

    fk::algorithms::klog("EXT2", "Mounted: block_size=%u inodes=%u blocks=%u groups=%u",
                         fs->m_block_size, sb.s_inodes_count, sb.s_blocks_count,
                         (sb.s_blocks_count + sb.s_blocks_per_group - 1)
                             / sb.s_blocks_per_group);
    return fs;
}

// ---- Block Group Descriptor --------------------------------------------------

uint32_t Ext2FileSystem::bg_of_inode(uint32_t ino) const {
    return (ino - 1) / m_super.s_inodes_per_group;
}

uint32_t Ext2FileSystem::local_idx_of_inode(uint32_t ino) const {
    return (ino - 1) % m_super.s_inodes_per_group;
}

fk::core::Result<void, Error>
Ext2FileSystem::read_bg_desc(uint32_t bg, Ext2BlockGroupDesc& out) {
    uint64_t off = static_cast<uint64_t>(m_bgdt_block) * m_block_size
                 + bg * sizeof(Ext2BlockGroupDesc);
    if (m_device->read(off, sizeof(out),
                       reinterpret_cast<uint8_t*>(&out)).is_error())
        return Error::IOError;
    return {};
}

fk::core::Result<void, Error>
Ext2FileSystem::write_bg_desc(uint32_t bg, const Ext2BlockGroupDesc& desc) {
    uint64_t off = static_cast<uint64_t>(m_bgdt_block) * m_block_size
                 + bg * sizeof(Ext2BlockGroupDesc);
    if (m_device->write(off, sizeof(desc),
                        reinterpret_cast<const uint8_t*>(&desc)).is_error())
        return Error::IOError;
    return {};
}

// ---- Inode I/O ---------------------------------------------------------------

fk::core::Result<void, Error>
Ext2FileSystem::read_inode(uint32_t ino, Ext2Inode& out) {
    if (ino == 0) return Error::InvalidData;
    Ext2BlockGroupDesc bg;
    TRY(read_bg_desc(bg_of_inode(ino), bg));

    uint32_t local  = local_idx_of_inode(ino);
    uint64_t offset = static_cast<uint64_t>(bg.bg_inode_table) * m_block_size
                    + static_cast<uint64_t>(local) * m_inode_size;

    fk::memory::set(&out, 0, sizeof(out));
    size_t copy_sz = m_inode_size < sizeof(Ext2Inode) ? m_inode_size : sizeof(Ext2Inode);
    if (m_device->read(offset, copy_sz, reinterpret_cast<uint8_t*>(&out)).is_error())
        return Error::IOError;
    return {};
}

fk::core::Result<void, Error>
Ext2FileSystem::write_inode(uint32_t ino, const Ext2Inode& inode) {
    if (ino == 0) return Error::InvalidData;
    Ext2BlockGroupDesc bg;
    TRY(read_bg_desc(bg_of_inode(ino), bg));

    uint32_t local  = local_idx_of_inode(ino);
    uint64_t offset = static_cast<uint64_t>(bg.bg_inode_table) * m_block_size
                    + static_cast<uint64_t>(local) * m_inode_size;

    size_t write_sz = m_inode_size < sizeof(Ext2Inode) ? m_inode_size : sizeof(Ext2Inode);
    if (m_device->write(offset, write_sz,
                        reinterpret_cast<const uint8_t*>(&inode)).is_error())
        return Error::IOError;
    return {};
}

// ---- Indirect block read helper ----------------------------------------------

static fk::core::Result<uint32_t, Error>
read_indirect(Ext2FileSystem* fs, uint32_t block, uint32_t index) {
    if (block == 0) return static_cast<uint32_t>(0);
    uint8_t* buf = static_cast<uint8_t*>(kmalloc(fs->m_block_size));
    if (!buf) return Error::OutOfMemory;
    auto res = fs->read_block(block, buf);
    if (res.is_error()) { kfree(buf); return res.error(); }
    uint32_t val;
    fk::memory::copy(&val, buf + index * 4, 4);
    kfree(buf);
    return val;
}

static fk::core::Result<void, Error>
write_indirect(Ext2FileSystem* fs, uint32_t block, uint32_t index, uint32_t value) {
    uint8_t* buf = static_cast<uint8_t*>(kmalloc(fs->m_block_size));
    if (!buf) return Error::OutOfMemory;
    auto res = fs->read_block(block, buf);
    if (res.is_error()) { kfree(buf); return res.error(); }
    fk::memory::copy(buf + index * 4, &value, 4);
    res = fs->write_block(block, buf);
    kfree(buf);
    return res;
}

// ---- Data block lookup (read path) ------------------------------------------

fk::core::Result<uint32_t, Error>
Ext2FileSystem::get_data_block(const Ext2Inode& inode, uint32_t logical) {
    if (logical < EXT2_NDIR_BLOCKS)
        return inode.i_block[logical];

    uint32_t idx = logical - EXT2_NDIR_BLOCKS;

    // Single indirect
    if (idx < m_ptrs_per_blk)
        return read_indirect(this, inode.i_block[12], idx);

    idx -= m_ptrs_per_blk;

    // Double indirect
    if (idx < m_ptrs_per_blk * m_ptrs_per_blk) {
        uint32_t outer_idx = idx / m_ptrs_per_blk;
        uint32_t inner_idx = idx % m_ptrs_per_blk;
        uint32_t outer = TRY(read_indirect(this, inode.i_block[13], outer_idx));
        return read_indirect(this, outer, inner_idx);
    }

    idx -= m_ptrs_per_blk * m_ptrs_per_blk;

    // Triple indirect
    uint32_t sq = m_ptrs_per_blk * m_ptrs_per_blk;
    uint32_t l1_idx = idx / sq;
    uint32_t l2_idx = (idx % sq) / m_ptrs_per_blk;
    uint32_t l3_idx = idx % m_ptrs_per_blk;
    uint32_t l1 = TRY(read_indirect(this, inode.i_block[14], l1_idx));
    uint32_t l2 = TRY(read_indirect(this, l1, l2_idx));
    return read_indirect(this, l2, l3_idx);
}

// ---- Data block set (write/alloc path) ----------------------------------------

// Ensures indirect block exists, allocating if needed. Returns the block number.
static fk::core::Result<uint32_t, Error>
ensure_indirect(Ext2FileSystem* fs, uint32_t& slot, uint32_t prefer_bg) {
    if (slot != 0) return slot;
    uint32_t new_blk = TRY(fs->alloc_block(prefer_bg));
    uint8_t* buf = static_cast<uint8_t*>(kmalloc(fs->m_block_size));
    if (!buf) { fs->free_block(new_blk); return Error::OutOfMemory; }
    fk::memory::set(buf, 0, fs->m_block_size);
    auto res = fs->write_block(new_blk, buf);
    kfree(buf);
    if (res.is_error()) { fs->free_block(new_blk); return res.error(); }
    slot = new_blk;
    return new_blk;
}

fk::core::Result<uint32_t, Error>
Ext2FileSystem::set_data_block(Ext2Inode& inode, uint32_t ino,
                                uint32_t logical, bool alloc) {
    uint32_t prefer = bg_of_inode(ino);

    if (logical < EXT2_NDIR_BLOCKS) {
        if (alloc && inode.i_block[logical] == 0) {
            inode.i_block[logical] = TRY(alloc_block(prefer));
            inode.i_blocks += m_block_size / 512;
        }
        return inode.i_block[logical];
    }

    uint32_t idx = logical - EXT2_NDIR_BLOCKS;

    if (idx < m_ptrs_per_blk) {
        uint32_t ind = TRY(ensure_indirect(this, inode.i_block[12], prefer));
        uint32_t leaf = TRY(read_indirect(this, ind, idx));
        if (alloc && leaf == 0) {
            leaf = TRY(alloc_block(prefer));
            TRY(write_indirect(this, ind, idx, leaf));
            inode.i_blocks += m_block_size / 512;
        }
        return leaf;
    }

    idx -= m_ptrs_per_blk;

    if (idx < m_ptrs_per_blk * m_ptrs_per_blk) {
        uint32_t outer_i = idx / m_ptrs_per_blk;
        uint32_t inner_i = idx % m_ptrs_per_blk;
        uint32_t dind = TRY(ensure_indirect(this, inode.i_block[13], prefer));
        uint32_t ind  = TRY(read_indirect(this, dind, outer_i));
        if (ind == 0) {
            ind = TRY(alloc_block(prefer));
            uint8_t* z = static_cast<uint8_t*>(kmalloc(m_block_size));
            if (!z) return Error::OutOfMemory;
            fk::memory::set(z, 0, m_block_size);
            auto r = write_block(ind, z); kfree(z);
            if (r.is_error()) return r.error();
            TRY(write_indirect(this, dind, outer_i, ind));
        }
        uint32_t leaf = TRY(read_indirect(this, ind, inner_i));
        if (alloc && leaf == 0) {
            leaf = TRY(alloc_block(prefer));
            TRY(write_indirect(this, ind, inner_i, leaf));
            inode.i_blocks += m_block_size / 512;
        }
        return leaf;
    }

    idx -= m_ptrs_per_blk * m_ptrs_per_blk;

    // Triple indirect: inode.i_block[14] → L1 → L2 → leaf
    uint32_t sq    = m_ptrs_per_blk * m_ptrs_per_blk;
    uint32_t l1_i  = idx / sq;
    uint32_t l2_i  = (idx % sq) / m_ptrs_per_blk;
    uint32_t l3_i  = idx % m_ptrs_per_blk;

    uint32_t tind = TRY(ensure_indirect(this, inode.i_block[14], prefer));

    uint32_t l1 = TRY(read_indirect(this, tind, l1_i));
    if (l1 == 0) {
        l1 = TRY(alloc_block(prefer));
        uint8_t* z = static_cast<uint8_t*>(kmalloc(m_block_size));
        if (!z) return Error::OutOfMemory;
        fk::memory::set(z, 0, m_block_size);
        auto r = write_block(l1, z); kfree(z);
        if (r.is_error()) return r.error();
        TRY(write_indirect(this, tind, l1_i, l1));
    }

    uint32_t l2 = TRY(read_indirect(this, l1, l2_i));
    if (l2 == 0) {
        l2 = TRY(alloc_block(prefer));
        uint8_t* z = static_cast<uint8_t*>(kmalloc(m_block_size));
        if (!z) return Error::OutOfMemory;
        fk::memory::set(z, 0, m_block_size);
        auto r = write_block(l2, z); kfree(z);
        if (r.is_error()) return r.error();
        TRY(write_indirect(this, l1, l2_i, l2));
    }

    uint32_t leaf = TRY(read_indirect(this, l2, l3_i));
    if (alloc && leaf == 0) {
        leaf = TRY(alloc_block(prefer));
        TRY(write_indirect(this, l2, l3_i, leaf));
        inode.i_blocks += m_block_size / 512;
    }
    return leaf;
}

// ---- Read from inode ---------------------------------------------------------

fk::core::Result<size_t, Error>
Ext2FileSystem::read_from_inode(const Ext2Inode& inode, uint64_t offset,
                                 size_t size, uint8_t* buf) {
    if (offset >= inode.i_size) return static_cast<size_t>(0);
    size_t remaining = inode.i_size - static_cast<size_t>(offset);
    if (size > remaining) size = remaining;
    if (size == 0) return static_cast<size_t>(0);

    uint8_t* block_buf = static_cast<uint8_t*>(kmalloc(m_block_size));
    if (!block_buf) return Error::OutOfMemory;

    auto resolve_block = [this, &inode](uint32_t lblock) -> fk::core::Result<uint64_t, Error> {
        auto res = get_data_block(inode, lblock);
        if (res.is_error()) return res.error();
        return static_cast<uint64_t>(res.value());
    };
    auto read_block_cb = [this](uint64_t phys, uint8_t* scratch) -> fk::core::Result<void, Error> {
        return read_block(static_cast<uint32_t>(phys), scratch);
    };

    auto res = fk::algorithms::scatter_read(offset, size, buf, m_block_size, block_buf,
                                            resolve_block, read_block_cb);
    kfree(block_buf);
    if (res.is_error()) return res.error();
    return res.value();
}

// ---- Directory operations ---------------------------------------------------

static bool ext2_name_equal(const char* stored, uint8_t stored_len, const char* lookup) {
    for (uint8_t i = 0; i < stored_len; ++i) {
        if (stored[i] != lookup[i]) return false;
    }
    return lookup[stored_len] == '\0';
}

static fk::RefPtr<Node> make_ext2_node(fk::RefPtr<Ext2FileSystem> fs,
                                        uint32_t ino, const Ext2Inode& inode) {
    auto node = fk::adopt_ref(new Ext2Node(fs, ino, inode));
    if (!node) return {};
    return fk::RefPtr<Node>(node.ptr());
}

fk::core::Result<void, Error>
Ext2FileSystem::list_dir_inode(const Ext2Inode& dir,
                                fk::containers::Vector<DirectoryEntry>& entries) {
    size_t dir_size = dir.i_size;
    size_t processed = 0;

    while (processed < dir_size) {
        uint32_t lblock = static_cast<uint32_t>(processed / m_block_size);
        uint32_t phys = TRY(get_data_block(dir, lblock));
        if (phys == 0) { processed += m_block_size; continue; }

        uint8_t* buf = static_cast<uint8_t*>(kmalloc(m_block_size));
        if (!buf) return Error::OutOfMemory;
        if (read_block(phys, buf).is_error()) { kfree(buf); return Error::IOError; }

        uint32_t off = static_cast<uint32_t>(processed % m_block_size);
        while (off + sizeof(Ext2DirEntry) <= m_block_size) {
            const Ext2DirEntry* de = reinterpret_cast<const Ext2DirEntry*>(buf + off);
            if (de->rec_len == 0) break;
            if (de->inode != 0 && de->name_len > 0) {
                const char* dname = reinterpret_cast<const char*>(buf + off + sizeof(Ext2DirEntry));
                if (!(de->name_len == 1 && dname[0] == '.') &&
                    !(de->name_len == 2 && dname[0] == '.' && dname[1] == '.')) {
                    DirectoryEntry entry;
                    size_t nlen = de->name_len < 255 ? de->name_len : 255u;
                    fk::memory::copy(entry.name, dname, nlen);
                    entry.name[nlen] = '\0';
                    entry.type = (de->file_type == EXT2_FT_DIR) ? 1u : 0u;
                    TRY(entries.push_back(entry));
                }
            }
            off += de->rec_len;
            processed += de->rec_len;
        }
        kfree(buf);
        // Align processed to next block boundary
        processed = (lblock + 1) * m_block_size;
    }
    return {};
}

fk::core::Result<fk::RefPtr<Node>, Error>
Ext2FileSystem::lookup_in_inode(const Ext2Inode& dir, const char* name) {
    size_t dir_size = dir.i_size;
    size_t processed = 0;

    while (processed < dir_size) {
        uint32_t lblock = static_cast<uint32_t>(processed / m_block_size);
        uint32_t phys = TRY(get_data_block(dir, lblock));
        if (phys == 0) { processed += m_block_size; continue; }

        uint8_t* buf = static_cast<uint8_t*>(kmalloc(m_block_size));
        if (!buf) return Error::OutOfMemory;
        if (read_block(phys, buf).is_error()) { kfree(buf); return Error::IOError; }

        uint32_t off = static_cast<uint32_t>(processed % m_block_size);
        while (off + sizeof(Ext2DirEntry) <= m_block_size) {
            const Ext2DirEntry* de = reinterpret_cast<const Ext2DirEntry*>(buf + off);
            if (de->rec_len == 0) break;
            if (de->inode != 0 && de->name_len > 0) {
                const char* dname = reinterpret_cast<const char*>(buf + off + sizeof(Ext2DirEntry));
                if (ext2_name_equal(dname, de->name_len, name)) {
                    uint32_t child_ino = de->inode;
                    kfree(buf);
                    Ext2Inode child;
                    TRY(read_inode(child_ino, child));
                    return make_ext2_node(fk::RefPtr<Ext2FileSystem>(this), child_ino, child);
                }
            }
            off += de->rec_len;
            processed += de->rec_len;
        }
        kfree(buf);
        processed = (lblock + 1) * m_block_size;
    }
    return Error::NotFound;
}

// ---- Block/inode allocation -------------------------------------------------

fk::core::Result<uint32_t, Error>
Ext2FileSystem::alloc_block(uint32_t prefer_bg) {
    uint32_t num_bgs = (m_super.s_blocks_count + m_super.s_blocks_per_group - 1)
                       / m_super.s_blocks_per_group;

    for (uint32_t pass = 0; pass < num_bgs; ++pass) {
        uint32_t bg = (prefer_bg + pass) % num_bgs;
        Ext2BlockGroupDesc desc;
        if (read_bg_desc(bg, desc).is_error()) continue;
        if (desc.bg_free_blocks_count == 0) continue;

        uint8_t* bitmap = static_cast<uint8_t*>(kmalloc(m_block_size));
        if (!bitmap) return Error::OutOfMemory;
        if (read_block(desc.bg_block_bitmap, bitmap).is_error()) { kfree(bitmap); continue; }

        size_t idx = fk::algorithms::find_first_free_bit(bitmap, m_block_size * 8);
        if (idx < m_block_size * 8) {
            fk::algorithms::set_bit(bitmap, idx);
            write_block(desc.bg_block_bitmap, bitmap);
            kfree(bitmap);
            desc.bg_free_blocks_count--;
            write_bg_desc(bg, desc);
            return m_super.s_first_data_block
                 + bg * m_super.s_blocks_per_group + idx;
        }
        kfree(bitmap);
    }
    return Error::OutOfMemory;
}

fk::core::Result<uint32_t, Error>
Ext2FileSystem::alloc_inode(bool is_dir) {
    uint32_t num_bgs = (m_super.s_blocks_count + m_super.s_blocks_per_group - 1)
                       / m_super.s_blocks_per_group;

    for (uint32_t bg = 0; bg < num_bgs; ++bg) {
        Ext2BlockGroupDesc desc;
        if (read_bg_desc(bg, desc).is_error()) continue;
        if (desc.bg_free_inodes_count == 0) continue;

        uint8_t* bitmap = static_cast<uint8_t*>(kmalloc(m_block_size));
        if (!bitmap) return Error::OutOfMemory;
        if (read_block(desc.bg_inode_bitmap, bitmap).is_error()) { kfree(bitmap); continue; }

        size_t idx = fk::algorithms::find_first_free_bit(bitmap, m_block_size * 8);
        if (idx < m_block_size * 8) {
            fk::algorithms::set_bit(bitmap, idx);
            write_block(desc.bg_inode_bitmap, bitmap);
            kfree(bitmap);
            desc.bg_free_inodes_count--;
            if (is_dir) desc.bg_used_dirs_count++;
            write_bg_desc(bg, desc);
            return bg * m_super.s_inodes_per_group + idx + 1;
        }
        kfree(bitmap);
    }
    return Error::OutOfMemory;
}

fk::core::Result<void, Error>
Ext2FileSystem::free_block(uint32_t block) {
    if (block < m_super.s_first_data_block) return {};
    uint32_t bg  = (block - m_super.s_first_data_block) / m_super.s_blocks_per_group;
    uint32_t bit = (block - m_super.s_first_data_block) % m_super.s_blocks_per_group;

    Ext2BlockGroupDesc desc;
    TRY(read_bg_desc(bg, desc));

    uint8_t* bitmap = static_cast<uint8_t*>(kmalloc(m_block_size));
    if (!bitmap) return Error::OutOfMemory;
    if (read_block(desc.bg_block_bitmap, bitmap).is_error()) { kfree(bitmap); return Error::IOError; }
    fk::algorithms::clear_bit(bitmap, bit);
    write_block(desc.bg_block_bitmap, bitmap);
    kfree(bitmap);
    desc.bg_free_blocks_count++;
    return write_bg_desc(bg, desc);
}

fk::core::Result<void, Error>
Ext2FileSystem::free_inode(uint32_t ino, bool is_dir) {
    uint32_t bg  = bg_of_inode(ino);
    uint32_t bit = local_idx_of_inode(ino);

    Ext2BlockGroupDesc desc;
    TRY(read_bg_desc(bg, desc));

    uint8_t* bitmap = static_cast<uint8_t*>(kmalloc(m_block_size));
    if (!bitmap) return Error::OutOfMemory;
    if (read_block(desc.bg_inode_bitmap, bitmap).is_error()) { kfree(bitmap); return Error::IOError; }
    fk::algorithms::clear_bit(bitmap, bit);
    write_block(desc.bg_inode_bitmap, bitmap);
    kfree(bitmap);
    desc.bg_free_inodes_count++;
    if (is_dir && desc.bg_used_dirs_count > 0) desc.bg_used_dirs_count--;
    return write_bg_desc(bg, desc);
}

// ---- Truncate (free data blocks) --------------------------------------------

fk::core::Result<void, Error>
Ext2FileSystem::truncate_inode(uint32_t ino, Ext2Inode& inode, uint64_t new_size) {
    uint32_t new_blocks  = static_cast<uint32_t>((new_size + m_block_size - 1) / m_block_size);
    uint32_t old_blocks  = (inode.i_size + m_block_size - 1) / m_block_size;

    for (uint32_t i = new_blocks; i < old_blocks && i < EXT2_NDIR_BLOCKS; ++i) {
        if (inode.i_block[i] != 0) {
            free_block(inode.i_block[i]);
            inode.i_block[i] = 0;
            if (inode.i_blocks >= m_block_size / 512)
                inode.i_blocks -= m_block_size / 512;
        }
    }
    // Indirect block freeing deferred — only handle direct blocks for now
    inode.i_size = static_cast<uint32_t>(new_size);
    return write_inode(ino, inode);
}

// ---- Create a directory entry -----------------------------------------------

static fk::core::Result<void, Error>
add_dir_entry(Ext2FileSystem* fs, uint32_t dir_ino, Ext2Inode& dir,
              uint32_t new_ino, const char* name, uint8_t file_type) {
    uint8_t name_len = static_cast<uint8_t>(fk::memory::length(name));
    uint16_t needed  = static_cast<uint16_t>(
        sizeof(Ext2DirEntry) + ((name_len + 3) & ~3u));

    size_t dir_size = dir.i_size;
    size_t processed = 0;

    while (processed < dir_size) {
        uint32_t lblock = static_cast<uint32_t>(processed / fs->m_block_size);
        uint32_t phys = TRY(fs->get_data_block(dir, lblock));
        if (phys == 0) { processed += fs->m_block_size; continue; }

        uint8_t* buf = static_cast<uint8_t*>(kmalloc(fs->m_block_size));
        if (!buf) return Error::OutOfMemory;
        if (fs->read_block(phys, buf).is_error()) { kfree(buf); return Error::IOError; }

        uint32_t off = static_cast<uint32_t>(processed % fs->m_block_size);
        while (off + sizeof(Ext2DirEntry) <= fs->m_block_size) {
            Ext2DirEntry* de = reinterpret_cast<Ext2DirEntry*>(buf + off);
            if (de->rec_len == 0) break;

            uint16_t actual = static_cast<uint16_t>(
                sizeof(Ext2DirEntry) + ((de->name_len + 3) & ~3u));
            uint16_t slack  = de->rec_len - actual;

            if (de->inode == 0 && de->rec_len >= needed) {
                de->inode     = new_ino;
                de->name_len  = name_len;
                de->file_type = file_type;
                fk::memory::copy(buf + off + sizeof(Ext2DirEntry), name, name_len);
                fs->write_block(phys, buf);
                kfree(buf);
                return {};
            }
            if (slack >= needed) {
                de->rec_len = actual;
                Ext2DirEntry* nd = reinterpret_cast<Ext2DirEntry*>(buf + off + actual);
                nd->inode     = new_ino;
                nd->rec_len   = slack;
                nd->name_len  = name_len;
                nd->file_type = file_type;
                fk::memory::copy(buf + off + actual + sizeof(Ext2DirEntry), name, name_len);
                fs->write_block(phys, buf);
                kfree(buf);
                return {};
            }
            off += de->rec_len;
            processed += de->rec_len;
        }
        kfree(buf);
        processed = (lblock + 1) * fs->m_block_size;
    }

    // Need a new block
    uint32_t new_lblock = static_cast<uint32_t>(dir.i_size / fs->m_block_size);
    uint32_t new_phys = TRY(fs->set_data_block(dir, dir_ino, new_lblock, true));
    uint8_t* buf = static_cast<uint8_t*>(kmalloc(fs->m_block_size));
    if (!buf) return Error::OutOfMemory;
    fk::memory::set(buf, 0, fs->m_block_size);

    Ext2DirEntry* de = reinterpret_cast<Ext2DirEntry*>(buf);
    de->inode     = new_ino;
    de->rec_len   = static_cast<uint16_t>(fs->m_block_size);
    de->name_len  = name_len;
    de->file_type = file_type;
    fk::memory::copy(buf + sizeof(Ext2DirEntry), name, name_len);
    fs->write_block(new_phys, buf);
    kfree(buf);
    dir.i_size += fs->m_block_size;
    return fs->write_inode(dir_ino, dir);
}

fk::core::Result<fk::RefPtr<Node>, Error>
Ext2FileSystem::create_in_dir(uint32_t dir_ino, Ext2Inode& dir,
                               const char* name, uint16_t mode) {
    bool is_dir  = (mode & 0170000u) == 0040000u;
    uint32_t new_ino = TRY(alloc_inode(is_dir));

    Ext2Inode new_inode{};
    new_inode.i_mode        = mode;
    new_inode.i_links_count = is_dir ? 2u : 1u;

    if (is_dir) {
        uint32_t dblk = TRY(alloc_block(bg_of_inode(new_ino)));
        uint8_t* buf  = static_cast<uint8_t*>(kmalloc(m_block_size));
        if (!buf) { free_inode(new_ino, true); return Error::OutOfMemory; }
        fk::memory::set(buf, 0, m_block_size);

        // "." entry
        Ext2DirEntry* de = reinterpret_cast<Ext2DirEntry*>(buf);
        de->inode = new_ino; de->rec_len = 12;
        de->name_len = 1; de->file_type = EXT2_FT_DIR;
        buf[sizeof(Ext2DirEntry)] = '.';

        // ".." entry
        Ext2DirEntry* de2 = reinterpret_cast<Ext2DirEntry*>(buf + 12);
        de2->inode = dir_ino;
        de2->rec_len = static_cast<uint16_t>(m_block_size - 12);
        de2->name_len = 2; de2->file_type = EXT2_FT_DIR;
        buf[12 + sizeof(Ext2DirEntry)] = '.';
        buf[12 + sizeof(Ext2DirEntry) + 1] = '.';

        write_block(dblk, buf);
        kfree(buf);
        new_inode.i_block[0] = dblk;
        new_inode.i_size     = m_block_size;
        new_inode.i_blocks   = m_block_size / 512;
        dir.i_links_count++;
    }

    TRY(write_inode(new_ino, new_inode));
    uint8_t ft = is_dir ? EXT2_FT_DIR : EXT2_FT_REG_FILE;
    TRY(add_dir_entry(this, dir_ino, dir, new_ino, name, ft));
    TRY(write_inode(dir_ino, dir));
    return make_ext2_node(fk::RefPtr<Ext2FileSystem>(this), new_ino, new_inode);
}

fk::core::Result<void, Error>
Ext2FileSystem::remove_from_dir(uint32_t dir_ino, Ext2Inode& dir,
                                 const char* name, bool check_empty) {
    size_t dir_size = dir.i_size;
    size_t processed = 0;

    while (processed < dir_size) {
        uint32_t lblock = static_cast<uint32_t>(processed / m_block_size);
        uint32_t phys = TRY(get_data_block(dir, lblock));
        if (phys == 0) { processed += m_block_size; continue; }

        uint8_t* buf = static_cast<uint8_t*>(kmalloc(m_block_size));
        if (!buf) return Error::OutOfMemory;
        if (read_block(phys, buf).is_error()) { kfree(buf); return Error::IOError; }

        uint32_t off = static_cast<uint32_t>(processed % m_block_size);
        while (off + sizeof(Ext2DirEntry) <= m_block_size) {
            Ext2DirEntry* de = reinterpret_cast<Ext2DirEntry*>(buf + off);
            if (de->rec_len == 0) break;
            if (de->inode != 0 && de->name_len > 0) {
                const char* dname = reinterpret_cast<const char*>(
                    buf + off + sizeof(Ext2DirEntry));
                if (ext2_name_equal(dname, de->name_len, name)) {
                    uint32_t child_ino = de->inode;
                    bool is_dir2 = (de->file_type == EXT2_FT_DIR);

                    Ext2Inode child;
                    if (read_inode(child_ino, child).is_error()) { kfree(buf); return Error::IOError; }
                    if (check_empty && child.i_size > m_block_size) {
                        kfree(buf); return Error::InvalidData;
                    }

                    // Zero inode field; rec_len stays so chain is preserved
                    de->inode = 0;
                    write_block(phys, buf);
                    kfree(buf);

                    if (child.i_links_count > 0) child.i_links_count--;
                    if (child.i_links_count == 0) {
                        truncate_inode(child_ino, child, 0);
                        free_inode(child_ino, is_dir2);
                    } else {
                        write_inode(child_ino, child);
                    }
                    if (is_dir2 && dir.i_links_count > 0) dir.i_links_count--;
                    return write_inode(dir_ino, dir);
                }
            }
            off += de->rec_len;
            processed += de->rec_len;
        }
        kfree(buf);
        processed = (lblock + 1) * m_block_size;
    }
    return Error::NotFound;
}

// ---- Root Node interface ----------------------------------------------------

fk::core::Result<size_t, Error>
Ext2FileSystem::read(uint64_t /*offset*/, size_t /*size*/, uint8_t* /*buffer*/) {
    return Error::IsDirectory;
}

fk::core::Result<size_t, Error>
Ext2FileSystem::write(uint64_t /*offset*/, size_t /*size*/, const uint8_t* /*buffer*/) {
    return Error::IsDirectory;
}

fk::core::Result<fk::RefPtr<Node>, Error>
Ext2FileSystem::lookup(const char* name) {
    Ext2Inode root;
    TRY(read_inode(EXT2_ROOT_INO, root));
    return lookup_in_inode(root, name);
}

fk::core::Result<void, Error>
Ext2FileSystem::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    Ext2Inode root;
    TRY(read_inode(EXT2_ROOT_INO, root));
    return list_dir_inode(root, entries);
}

fk::core::Result<fk::RefPtr<Node>, Error>
Ext2FileSystem::create_child(const char* name, int mode) {
    Ext2Inode root;
    TRY(read_inode(EXT2_ROOT_INO, root));
    return create_in_dir(EXT2_ROOT_INO, root, name,
                         static_cast<uint16_t>(0100000u | (mode & 0777u)));
}

fk::core::Result<fk::RefPtr<Node>, Error>
Ext2FileSystem::mkdir(const char* name, int mode) {
    Ext2Inode root;
    TRY(read_inode(EXT2_ROOT_INO, root));
    return create_in_dir(EXT2_ROOT_INO, root, name,
                         static_cast<uint16_t>(0040000u | (mode & 0777u)));
}

fk::core::Result<void, Error>
Ext2FileSystem::unlink(const char* name) {
    Ext2Inode root;
    TRY(read_inode(EXT2_ROOT_INO, root));
    return remove_from_dir(EXT2_ROOT_INO, root, name, false);
}

fk::core::Result<void, Error>
Ext2FileSystem::rmdir(const char* name) {
    Ext2Inode root;
    TRY(read_inode(EXT2_ROOT_INO, root));
    return remove_from_dir(EXT2_ROOT_INO, root, name, true);
}

} // namespace fkernel
