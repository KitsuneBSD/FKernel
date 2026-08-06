#include <LibFK/Algorithms/Generic/bitmap.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>

#include <Kernel/Fs/Disk/MinixFs/minix_fs.h>
#include <Kernel/Fs/Disk/MinixFs/minix_node.h>

namespace fkernel {

using fk::core::Error;
using fk::core::Result;

// ---- Block I/O ----------------------------------------------------------------

fk::core::Result<void, Error>
MinixFileSystem::read_block(uint32_t block, uint8_t* buf) {
    auto res = m_device->read(static_cast<uint64_t>(block) * MINIX_BLOCK_SIZE,
                              MINIX_BLOCK_SIZE, buf);
    if (res.is_error()) return Error::IOError;
    return {};
}

fk::core::Result<void, Error>
MinixFileSystem::write_block(uint32_t block, const uint8_t* buf) {
    auto res = m_device->write(static_cast<uint64_t>(block) * MINIX_BLOCK_SIZE,
                               MINIX_BLOCK_SIZE, buf);
    if (res.is_error()) return Error::IOError;
    return {};
}

// ---- create() factory ---------------------------------------------------------

fk::core::Result<fk::RefPtr<MinixFileSystem>, Error>
MinixFileSystem::create(fk::RefPtr<StorageDevice> device) {
    MinixSuperBlock sb;
    // Superblock sits at block 1 (byte offset 1024)
    if (device->read(MINIX_BLOCK_SIZE, sizeof(sb),
                     reinterpret_cast<uint8_t*>(&sb)).is_error())
        return Error::IOError;

    if (sb.s_magic != MINIX1_MAGIC && sb.s_magic != MINIX1_MAGIC30) {
        return Error::InvalidData;
    }

    auto fs = fk::adopt_ref(new MinixFileSystem(device));
    if (!fs) return Error::OutOfMemory;

    fs->m_super          = sb;
    fs->m_name_len       = (sb.s_magic == MINIX1_MAGIC30) ? 30u : 14u;
    fs->m_inode_table_blk = 2u + sb.s_imap_blocks + sb.s_zmap_blocks;

    fk::algorithms::klog("MINIXFS", "Mounted: ninodes=%u nzones=%u firstdatazone=%u",
                         sb.s_ninodes, sb.s_nzones, sb.s_firstdatazone);
    return fs;
}

// ---- Inode I/O ---------------------------------------------------------------

fk::core::Result<void, Error>
MinixFileSystem::read_inode(uint32_t ino, MinixInode& out) {
    if (ino == 0 || ino > m_super.s_ninodes) return Error::InvalidData;

    uint32_t idx   = ino - 1;
    uint32_t block = m_inode_table_blk + idx / MINIX_INODES_PER_BLOCK;
    uint32_t off   = (idx % MINIX_INODES_PER_BLOCK) * sizeof(MinixInode);

    uint8_t buf[MINIX_BLOCK_SIZE];
    TRY(read_block(block, buf));
    fk::memory::copy(&out, buf + off, sizeof(MinixInode));
    return {};
}

fk::core::Result<void, Error>
MinixFileSystem::write_inode(uint32_t ino, const MinixInode& inode) {
    if (ino == 0 || ino > m_super.s_ninodes) return Error::InvalidData;

    uint32_t idx   = ino - 1;
    uint32_t block = m_inode_table_blk + idx / MINIX_INODES_PER_BLOCK;
    uint32_t off   = (idx % MINIX_INODES_PER_BLOCK) * sizeof(MinixInode);

    uint8_t buf[MINIX_BLOCK_SIZE];
    TRY(read_block(block, buf));
    fk::memory::copy(buf + off, &inode, sizeof(MinixInode));
    return write_block(block, buf);
}

// ---- Zone resolution ---------------------------------------------------------

static fk::core::Result<uint16_t, Error>
read_indirect_zone(MinixFileSystem* fs, uint16_t indirect_zone,
                   uint32_t index) {
    if (indirect_zone == 0) return static_cast<uint16_t>(0);
    uint8_t buf[MINIX_BLOCK_SIZE];
    TRY(fs->read_block(indirect_zone, buf));
    uint16_t zone;
    fk::memory::copy(&zone, buf + index * 2, 2);
    return zone;
}

fk::core::Result<uint16_t, Error>
MinixFileSystem::get_data_zone(const MinixInode& inode, uint32_t zone_index) {
    if (zone_index < MINIX_DIRECT_ZONES)
        return inode.i_zone[zone_index];

    uint32_t idx = zone_index - MINIX_DIRECT_ZONES;
    if (idx < MINIX_PTR_PER_BLOCK) {
        return read_indirect_zone(this, inode.i_zone[7], idx);
    }

    idx -= MINIX_PTR_PER_BLOCK;
    uint32_t outer = idx / MINIX_PTR_PER_BLOCK;
    uint32_t inner = idx % MINIX_PTR_PER_BLOCK;
    uint16_t outer_zone = TRY(read_indirect_zone(this, inode.i_zone[8], outer));
    return read_indirect_zone(this, outer_zone, inner);
}

// ---- Data reading from inode -------------------------------------------------

fk::core::Result<size_t, Error>
MinixFileSystem::read_from_inode(const MinixInode& inode, uint64_t offset,
                                  size_t size, uint8_t* buf) {
    if (offset >= inode.i_size) return static_cast<size_t>(0);
    size_t remaining = inode.i_size - static_cast<size_t>(offset);
    if (size > remaining) size = remaining;

    size_t zsize    = zone_size();
    size_t total    = 0;
    uint64_t cur    = offset;

    while (total < size) {
        uint32_t zone_idx   = static_cast<uint32_t>(cur / zsize);
        uint32_t zone_off   = static_cast<uint32_t>(cur % zsize);
        size_t   chunk      = zsize - zone_off;
        if (chunk > size - total) chunk = size - total;

        uint16_t zone = TRY(get_data_zone(inode, zone_idx));
        if (zone == 0) {
            fk::memory::set(buf + total, 0, chunk);
        } else {
            uint8_t zbuf[MINIX_BLOCK_SIZE];
            TRY(read_block(zone, zbuf));
            fk::memory::copy(buf + total, zbuf + zone_off, chunk);
        }
        total += chunk;
        cur   += chunk;
    }
    return total;
}

// ---- Directory helpers -------------------------------------------------------

static bool name_matches(const char* entry_name, size_t max_len,
                         const char* lookup) {
    for (size_t i = 0; i < max_len; ++i) {
        if (entry_name[i] == '\0' && lookup[i] == '\0') return true;
        if (entry_name[i] != lookup[i]) return false;
    }
    return lookup[max_len] == '\0';
}

static fk::RefPtr<Node> make_minix_node(fk::RefPtr<MinixFileSystem> fs,
                                         uint32_t ino,
                                         const MinixInode& inode) {
    auto node = fk::adopt_ref(new MinixNode(fs, ino, inode));
    if (!node) return {};
    return fk::RefPtr<Node>(node.ptr());
}

fk::core::Result<void, Error>
MinixFileSystem::list_dir_inode(const MinixInode& inode,
                                 fk::containers::Vector<DirectoryEntry>& entries) {
    size_t entry_sz   = dir_entry_size();
    size_t entries_per_zone = zone_size() / entry_sz;
    uint32_t num_zones = (inode.i_size + zone_size() - 1) / zone_size();

    for (uint32_t zi = 0; zi < num_zones; ++zi) {
        uint16_t zone = TRY(get_data_zone(inode, zi));
        if (zone == 0) continue;

        uint8_t zbuf[MINIX_BLOCK_SIZE];
        TRY(read_block(zone, zbuf));

        for (size_t ei = 0; ei < entries_per_zone; ++ei) {
            const uint8_t* ep = zbuf + ei * entry_sz;
            uint16_t dino;
            fk::memory::copy(&dino, ep, 2);
            if (dino == 0) continue;

            const char* dname = reinterpret_cast<const char*>(ep + 2);
            if (dname[0] == '.' &&
                (dname[1] == '\0' || (dname[1] == '.' && dname[2] == '\0')))
                continue;

            DirectoryEntry de;
            size_t copy_len = m_name_len < 255 ? m_name_len : 255u;
            fk::memory::copy(de.name, dname, copy_len);
            de.name[copy_len] = '\0';

            MinixInode child_ino;
            if (read_inode(dino, child_ino).is_error()) continue;
            de.type = ((child_ino.i_mode & 0170000u) == 0040000u) ? 1u : 0u;
            TRY(entries.push_back(de));
        }
    }
    return {};
}

fk::core::Result<fk::RefPtr<Node>, Error>
MinixFileSystem::lookup_in_inode(uint32_t /*dir_ino*/, const MinixInode& dir_inode,
                                  const char* name) {
    size_t entry_sz = dir_entry_size();
    size_t entries_per_zone = zone_size() / entry_sz;
    uint32_t num_zones = (dir_inode.i_size + zone_size() - 1) / zone_size();

    for (uint32_t zi = 0; zi < num_zones; ++zi) {
        uint16_t zone = TRY(get_data_zone(dir_inode, zi));
        if (zone == 0) continue;

        uint8_t zbuf[MINIX_BLOCK_SIZE];
        TRY(read_block(zone, zbuf));

        for (size_t ei = 0; ei < entries_per_zone; ++ei) {
            const uint8_t* ep = zbuf + ei * entry_sz;
            uint16_t dino;
            fk::memory::copy(&dino, ep, 2);
            if (dino == 0) continue;

            const char* dname = reinterpret_cast<const char*>(ep + 2);
            if (!name_matches(dname, m_name_len, name)) continue;

            MinixInode child_inode;
            TRY(read_inode(dino, child_inode));
            return make_minix_node(fk::RefPtr<MinixFileSystem>(this), dino, child_inode);
        }
    }
    return Error::NotFound;
}

// ---- Bitmap allocation -------------------------------------------------------

static fk::core::Result<uint32_t, Error>
scan_bitmap(MinixFileSystem* fs, uint32_t first_block,
            uint32_t num_blocks, uint32_t first_index) {
    for (uint32_t bi = 0; bi < num_blocks; ++bi) {
        uint8_t buf[MINIX_BLOCK_SIZE];
        TRY(fs->read_block(first_block + bi, buf));

        size_t bits_per_block = MINIX_BLOCK_SIZE * 8;
        size_t idx = fk::algorithms::find_first_free_bit(buf, bits_per_block);
        if (idx < bits_per_block) {
            fk::algorithms::set_bit(buf, idx);
            TRY(fs->write_block(first_block + bi, buf));
            return first_index + bi * bits_per_block + idx;
        }
    }
    return Error::OutOfMemory;
}

static fk::core::Result<void, Error>
clear_bitmap(MinixFileSystem* fs, uint32_t first_block,
             uint32_t num_blocks, uint32_t index, uint32_t first_index) {
    (void)num_blocks;
    uint32_t bit_index = index - first_index;
    uint32_t block_off = bit_index / (MINIX_BLOCK_SIZE * 8);
    uint32_t bit_off   = bit_index % (MINIX_BLOCK_SIZE * 8);

    uint8_t buf[MINIX_BLOCK_SIZE];
    TRY(fs->read_block(first_block + block_off, buf));
    fk::algorithms::clear_bit(buf, bit_off);
    return fs->write_block(first_block + block_off, buf);
}

fk::core::Result<uint32_t, Error>
MinixFileSystem::allocate_inode_num() {
    // Inode bitmap starts at block 2; inodes are 1-indexed
    auto ino = TRY(scan_bitmap(this, 2, m_super.s_imap_blocks, 1));
    if (ino > m_super.s_ninodes) return Error::OutOfMemory;
    return ino;
}

fk::core::Result<uint16_t, Error>
MinixFileSystem::allocate_zone_num() {
    uint32_t first_block = 2u + m_super.s_imap_blocks;
    auto zone = TRY(scan_bitmap(this, first_block, m_super.s_zmap_blocks,
                                m_super.s_firstdatazone));
    if (zone > m_super.s_nzones) return Error::OutOfMemory;
    return static_cast<uint16_t>(zone);
}

fk::core::Result<void, Error>
MinixFileSystem::free_inode_num(uint32_t ino) {
    return clear_bitmap(this, 2, m_super.s_imap_blocks, ino, 1);
}

fk::core::Result<void, Error>
MinixFileSystem::free_zone_num(uint16_t zone) {
    uint32_t first_block = 2u + m_super.s_imap_blocks;
    return clear_bitmap(this, first_block, m_super.s_zmap_blocks,
                        zone, m_super.s_firstdatazone);
}

// ---- Write directory entry --------------------------------------------------

static fk::core::Result<void, Error>
write_dir_entry(MinixFileSystem* fs, MinixInode& dir_inode, uint32_t dir_ino,
                uint16_t new_ino, const char* name) {
    size_t entry_sz         = fs->dir_entry_size();
    size_t entries_per_zone = fs->zone_size() / entry_sz;
    uint32_t num_zones = (dir_inode.i_size + fs->zone_size() - 1) / fs->zone_size();

    // Find a free slot in existing zones
    for (uint32_t zi = 0; zi < num_zones; ++zi) {
        uint16_t zone;
        auto zr = fs->get_data_zone(dir_inode, zi);
        if (zr.is_error() || (zone = zr.value()) == 0) continue;

        uint8_t zbuf[MINIX_BLOCK_SIZE];
        if (fs->read_block(zone, zbuf).is_error()) continue;

        for (size_t ei = 0; ei < entries_per_zone; ++ei) {
            uint8_t* ep = zbuf + ei * entry_sz;
            uint16_t dino;
            fk::memory::copy(&dino, ep, 2);
            if (dino != 0) continue;

            fk::memory::copy(ep, &new_ino, 2);
            size_t nlen = fk::memory::length(name);
            size_t copy = nlen < (entry_sz - 2) ? nlen : (entry_sz - 2);
            fk::memory::set(ep + 2, 0, entry_sz - 2);
            fk::memory::copy(ep + 2, name, copy);
            return fs->write_block(zone, zbuf);
        }
    }

    // Allocate a new zone for the directory
    uint16_t new_zone = TRY(fs->allocate_zone_num());
    uint8_t zbuf[MINIX_BLOCK_SIZE];
    fk::memory::set(zbuf, 0, MINIX_BLOCK_SIZE);
    fk::memory::copy(zbuf, &new_ino, 2);
    size_t nlen = fk::memory::length(name);
    size_t copy = nlen < (entry_sz - 2) ? nlen : (entry_sz - 2);
    fk::memory::copy(zbuf + 2, name, copy);
    TRY(fs->write_block(new_zone, zbuf));

    uint32_t new_zone_idx = dir_inode.i_size / fs->zone_size();
    if (new_zone_idx < MINIX_DIRECT_ZONES) {
        dir_inode.i_zone[new_zone_idx] = new_zone;
    }
    dir_inode.i_size += fs->zone_size();
    return fs->write_inode(dir_ino, dir_inode);
}

// ---- create_in_inode ---------------------------------------------------------

fk::core::Result<fk::RefPtr<Node>, Error>
MinixFileSystem::create_in_inode(uint32_t dir_ino, MinixInode& dir_inode,
                                  const char* name, uint16_t mode) {
    uint32_t new_ino = TRY(allocate_inode_num());

    MinixInode new_inode{};
    new_inode.i_mode   = mode;
    new_inode.i_nlinks = 1;
    new_inode.i_size   = 0;

    bool is_dir = (mode & 0170000u) == 0040000u;
    if (is_dir) {
        // Allocate a zone for "." and ".."
        uint16_t dzone = TRY(allocate_zone_num());
        uint8_t zbuf[MINIX_BLOCK_SIZE];
        fk::memory::set(zbuf, 0, MINIX_BLOCK_SIZE);
        size_t entry_sz = dir_entry_size();
        uint16_t self_ino  = static_cast<uint16_t>(new_ino);
        uint16_t par_ino   = static_cast<uint16_t>(dir_ino);
        fk::memory::copy(zbuf,              &self_ino, 2);
        zbuf[2] = '.';
        fk::memory::copy(zbuf + entry_sz,   &par_ino,  2);
        zbuf[entry_sz + 2] = '.'; zbuf[entry_sz + 3] = '.';
        TRY(write_block(dzone, zbuf));
        new_inode.i_zone[0] = dzone;
        new_inode.i_size    = zone_size();
        new_inode.i_nlinks  = 2;
        dir_inode.i_nlinks++;
    }

    TRY(write_inode(static_cast<uint32_t>(new_ino), new_inode));
    TRY(write_dir_entry(this, dir_inode, dir_ino,
                        static_cast<uint16_t>(new_ino), name));
    TRY(write_inode(dir_ino, dir_inode));

    return make_minix_node(fk::RefPtr<MinixFileSystem>(this), new_ino, new_inode);
}

// ---- remove_from_inode -------------------------------------------------------

fk::core::Result<void, Error>
MinixFileSystem::remove_from_inode(uint32_t dir_ino, MinixInode& dir_inode,
                                    const char* name, bool check_empty) {
    size_t entry_sz         = dir_entry_size();
    size_t entries_per_zone = zone_size() / entry_sz;
    uint32_t num_zones = (dir_inode.i_size + zone_size() - 1) / zone_size();

    for (uint32_t zi = 0; zi < num_zones; ++zi) {
        uint16_t zone = TRY(get_data_zone(dir_inode, zi));
        if (zone == 0) continue;

        uint8_t zbuf[MINIX_BLOCK_SIZE];
        TRY(read_block(zone, zbuf));

        for (size_t ei = 0; ei < entries_per_zone; ++ei) {
            uint8_t* ep = zbuf + ei * entry_sz;
            uint16_t dino;
            fk::memory::copy(&dino, ep, 2);
            if (dino == 0) continue;

            const char* dname = reinterpret_cast<const char*>(ep + 2);
            if (!name_matches(dname, m_name_len, name)) continue;

            if (check_empty) {
                MinixInode child;
                TRY(read_inode(dino, child));
                if (child.i_size > zone_size()) return Error::InvalidData; // not empty
            }

            // Zero the entry and decrement nlinks
            fk::memory::set(ep, 0, entry_sz);
            TRY(write_block(zone, zbuf));

            MinixInode child;
            TRY(read_inode(dino, child));
            if (child.i_nlinks > 0) child.i_nlinks--;
            if (child.i_nlinks == 0) {
                TRY(truncate_inode(dino, child, 0));
                TRY(free_inode_num(dino));
            } else {
                TRY(write_inode(dino, child));
            }
            if (check_empty && dir_inode.i_nlinks > 0) dir_inode.i_nlinks--;
            return write_inode(dir_ino, dir_inode);
        }
    }
    return Error::NotFound;
}

// ---- truncate_inode ----------------------------------------------------------

fk::core::Result<void, Error>
MinixFileSystem::truncate_inode(uint32_t ino, MinixInode& inode, uint64_t new_size) {
    uint32_t zsize      = zone_size();
    uint32_t new_zones  = static_cast<uint32_t>((new_size + zsize - 1) / zsize);
    uint32_t old_zones  = (inode.i_size + zsize - 1) / zsize;

    for (uint32_t zi = new_zones; zi < old_zones && zi < MINIX_DIRECT_ZONES; ++zi) {
        if (inode.i_zone[zi] != 0) {
            free_zone_num(inode.i_zone[zi]);
            inode.i_zone[zi] = 0;
        }
    }
    inode.i_size = static_cast<uint32_t>(new_size);
    return write_inode(ino, inode);
}

// ---- Root Node interface ----------------------------------------------------

fk::core::Result<size_t, Error>
MinixFileSystem::read(uint64_t /*offset*/, size_t /*size*/, uint8_t* /*buffer*/) {
    return Error::IsDirectory;
}

fk::core::Result<size_t, Error>
MinixFileSystem::write(uint64_t /*offset*/, size_t /*size*/, const uint8_t* /*buffer*/) {
    return Error::IsDirectory;
}

fk::core::Result<fk::RefPtr<Node>, Error>
MinixFileSystem::lookup(const char* name) {
    MinixInode root;
    TRY(read_inode(1, root));
    return lookup_in_inode(1, root, name);
}

fk::core::Result<void, Error>
MinixFileSystem::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    MinixInode root;
    TRY(read_inode(1, root));
    return list_dir_inode(root, entries);
}

fk::core::Result<fk::RefPtr<Node>, Error>
MinixFileSystem::create_child(const char* name, int mode) {
    MinixInode root;
    TRY(read_inode(1, root));
    return create_in_inode(1, root, name,
                           static_cast<uint16_t>(0100000u | (mode & 0777u)));
}

fk::core::Result<fk::RefPtr<Node>, Error>
MinixFileSystem::mkdir(const char* name, int mode) {
    MinixInode root;
    TRY(read_inode(1, root));
    return create_in_inode(1, root, name,
                           static_cast<uint16_t>(0040000u | (mode & 0777u)));
}

fk::core::Result<void, Error>
MinixFileSystem::unlink(const char* name) {
    MinixInode root;
    TRY(read_inode(1, root));
    return remove_from_inode(1, root, name, false);
}

fk::core::Result<void, Error>
MinixFileSystem::rmdir(const char* name) {
    MinixInode root;
    TRY(read_inode(1, root));
    return remove_from_inode(1, root, name, true);
}

} // namespace fkernel
