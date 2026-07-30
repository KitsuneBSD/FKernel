#include <Kernel/Fs/Disk/Ufs/ufs_fs.h>
#include <Kernel/Fs/Disk/Ufs/ufs_node.h>
#include <Kernel/Fs/Disk/Ufs/ufs_dir.h>
#include <Kernel/Fs/Disk/Ufs/ufs_endian.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Memory/heap_malloc.h>

namespace fkernel {

using fk::core::Error;

// ---- Factory -----------------------------------------------------------------

fk::core::Result<fk::RefPtr<UfsFileSystem>, Error>
UfsFileSystem::create(fk::RefPtr<StorageDevice> device) {
    uint8_t raw[UFS_SBSIZE];
    if (device->read(UFS_SBOFF, UFS_SBSIZE, raw).is_error())
        return Error::IOError;

    auto read32 = [&](uint32_t off) -> uint32_t {
        uint32_t v;
        fk::memory::copy(&v, raw + off, 4);
        return ufs_le32(v);
    };

    uint32_t magic = read32(UFS_OFF_MAGIC);
    bool is_ufs2 = (magic == UFS2_MAGIC);
    if (magic != UFS1_MAGIC && !is_ufs2)
        return Error::InvalidData;

    UfsMountInfo info;
    info.is_ufs2 = is_ufs2;
    info.bsize   = read32(UFS_OFF_BSIZE);
    info.fsize   = read32(UFS_OFF_FSIZE);
    info.frag    = read32(UFS_OFF_FRAG);
    info.nindir  = read32(UFS_OFF_NINDIR);
    info.inopb   = read32(UFS_OFF_INOPB);
    info.ncg     = read32(UFS_OFF_NCG);
    info.ipg     = read32(UFS_OFF_IPG);
    info.fpg     = read32(UFS_OFF_FPG);
    info.iblkno  = read32(UFS_OFF_IBLKNO);
    info.fsbtodb = read32(UFS_OFF_FSBTODB);

    if (info.bsize == 0 || info.fsize == 0 || info.ipg == 0 || info.fpg == 0)
        return Error::InvalidData;

    auto fs = fk::adopt_ref(new UfsFileSystem());
    if (!fs) return Error::OutOfMemory;
    fs->m_device = device;
    fs->m_info   = info;

    fk::algorithms::klog("UFS", "Mounted UFS%s: bsize=%u fsize=%u ncg=%u ipg=%u",
                         is_ufs2 ? "2" : "1",
                         info.bsize, info.fsize, info.ncg, info.ipg);
    return fs;
}

// ---- Inode I/O ---------------------------------------------------------------

fk::core::Result<void, Error>
UfsFileSystem::read_inode1(uint32_t ino, Ufs1Dinode& out) {
    uint32_t cg     = ino / m_info.ipg;
    uint64_t offset = (static_cast<uint64_t>(cg) * m_info.fpg + m_info.iblkno)
                      * m_info.fsize
                      + static_cast<uint64_t>(ino % m_info.ipg) * sizeof(Ufs1Dinode);
    fk::memory::set(&out, 0, sizeof(out));
    if (m_device->read(offset, sizeof(out), reinterpret_cast<uint8_t*>(&out)).is_error())
        return Error::IOError;
    return {};
}

fk::core::Result<void, Error>
UfsFileSystem::read_inode2(uint32_t ino, Ufs2Dinode& out) {
    uint32_t cg     = ino / m_info.ipg;
    uint64_t offset = (static_cast<uint64_t>(cg) * m_info.fpg + m_info.iblkno)
                      * m_info.fsize
                      + static_cast<uint64_t>(ino % m_info.ipg) * sizeof(Ufs2Dinode);
    fk::memory::set(&out, 0, sizeof(out));
    if (m_device->read(offset, sizeof(out), reinterpret_cast<uint8_t*>(&out)).is_error())
        return Error::IOError;
    return {};
}

// ---- Indirect block helpers --------------------------------------------------

static fk::core::Result<uint32_t, Error>
read_ind32(StorageDevice* dev, uint64_t frag_byte, uint32_t index) {
    uint32_t v;
    uint64_t off = frag_byte + static_cast<uint64_t>(index) * 4;
    if (dev->read(off, 4, reinterpret_cast<uint8_t*>(&v)).is_error())
        return Error::IOError;
    return ufs_le32(v);
}

static fk::core::Result<uint64_t, Error>
read_ind64(StorageDevice* dev, uint64_t frag_byte, uint32_t index) {
    uint64_t v;
    uint64_t off = frag_byte + static_cast<uint64_t>(index) * 8;
    if (dev->read(off, 8, reinterpret_cast<uint8_t*>(&v)).is_error())
        return Error::IOError;
    return ufs_le64(v);
}

// ---- Block address lookup (UFS1) ---------------------------------------------

fk::core::Result<uint64_t, Error>
UfsFileSystem::get_block_frag(uint32_t ino, uint32_t logical) {
    Ufs1Dinode dino;
    TRY(read_inode1(ino, dino));

    uint32_t nindir = m_info.nindir;

    auto f2b = [&](int32_t f) -> uint64_t {
        return static_cast<uint64_t>(static_cast<uint32_t>(f)) * m_info.fsize;
    };

    if (logical < 12)
        return f2b(dino.di_db[logical]);

    uint32_t idx = logical - 12;

    if (idx < nindir) {
        uint32_t leaf = TRY(read_ind32(m_device.ptr(), f2b(dino.di_ib[0]), idx));
        return static_cast<uint64_t>(leaf) * m_info.fsize;
    }
    idx -= nindir;

    if (idx < nindir * nindir) {
        uint32_t l1 = idx / nindir;
        uint32_t l2 = idx % nindir;
        uint32_t dind = TRY(read_ind32(m_device.ptr(), f2b(dino.di_ib[1]), l1));
        uint32_t leaf = TRY(read_ind32(m_device.ptr(),
                                        static_cast<uint64_t>(dind) * m_info.fsize, l2));
        return static_cast<uint64_t>(leaf) * m_info.fsize;
    }
    idx -= nindir * nindir;

    uint32_t sq = nindir * nindir;
    uint32_t l1 = idx / sq;
    uint32_t l2 = (idx % sq) / nindir;
    uint32_t l3 = idx % nindir;
    uint32_t t    = TRY(read_ind32(m_device.ptr(), f2b(dino.di_ib[2]), l1));
    uint32_t d    = TRY(read_ind32(m_device.ptr(), static_cast<uint64_t>(t) * m_info.fsize, l2));
    uint32_t leaf = TRY(read_ind32(m_device.ptr(), static_cast<uint64_t>(d) * m_info.fsize, l3));
    return static_cast<uint64_t>(leaf) * m_info.fsize;
}

// ---- Block address lookup (UFS2) ---------------------------------------------

fk::core::Result<uint64_t, Error>
UfsFileSystem::get_block_frag2(uint64_t ino, uint32_t logical) {
    Ufs2Dinode dino;
    TRY(read_inode2(static_cast<uint32_t>(ino), dino));

    uint32_t nindir = m_info.nindir;

    auto f2b = [&](int64_t f) -> uint64_t {
        return static_cast<uint64_t>(f) * m_info.fsize;
    };

    if (logical < 12)
        return f2b(dino.di_db[logical]);

    uint32_t idx = logical - 12;

    if (idx < nindir) {
        uint64_t leaf = TRY(read_ind64(m_device.ptr(), f2b(dino.di_ib[0]), idx));
        return leaf * m_info.fsize;
    }
    idx -= nindir;

    if (idx < nindir * nindir) {
        uint32_t l1 = idx / nindir;
        uint32_t l2 = idx % nindir;
        uint64_t dind = TRY(read_ind64(m_device.ptr(), f2b(dino.di_ib[1]), l1));
        uint64_t leaf = TRY(read_ind64(m_device.ptr(), dind * m_info.fsize, l2));
        return leaf * m_info.fsize;
    }
    idx -= nindir * nindir;

    uint32_t sq = nindir * nindir;
    uint32_t l1 = idx / sq;
    uint32_t l2 = (idx % sq) / nindir;
    uint32_t l3 = idx % nindir;
    uint64_t t    = TRY(read_ind64(m_device.ptr(), f2b(dino.di_ib[2]), l1));
    uint64_t d    = TRY(read_ind64(m_device.ptr(), t * m_info.fsize, l2));
    uint64_t leaf = TRY(read_ind64(m_device.ptr(), d * m_info.fsize, l3));
    return leaf * m_info.fsize;
}

// ---- Read bytes from inode ---------------------------------------------------

fk::core::Result<size_t, Error>
UfsFileSystem::read_from_ino(uint32_t ino, uint64_t offset, size_t size, uint8_t* buf) {
    uint64_t file_size;
    if (m_info.is_ufs2) {
        Ufs2Dinode dino;
        TRY(read_inode2(ino, dino));
        file_size = dino.di_size;
    } else {
        Ufs1Dinode dino;
        TRY(read_inode1(ino, dino));
        file_size = dino.di_size;
    }

    if (offset >= file_size) return static_cast<size_t>(0);
    size_t remaining = static_cast<size_t>(file_size - offset);
    if (size > remaining) size = remaining;

    size_t total = 0;
    uint64_t cur = offset;

    while (total < size) {
        uint32_t lblock  = static_cast<uint32_t>(cur / m_info.bsize);
        uint32_t blk_off = static_cast<uint32_t>(cur % m_info.bsize);
        size_t   chunk   = m_info.bsize - blk_off;
        if (chunk > size - total) chunk = size - total;

        uint64_t phys_byte;
        if (m_info.is_ufs2) {
            phys_byte = TRY(get_block_frag2(ino, lblock));
        } else {
            phys_byte = TRY(get_block_frag(ino, lblock));
        }

        if (phys_byte == 0) {
            fk::memory::set(buf + total, 0, chunk);
        } else {
            if (m_device->read(phys_byte + blk_off, chunk, buf + total).is_error())
                return Error::IOError;
        }
        total += chunk;
        cur   += chunk;
    }
    return total;
}

// ---- Directory iteration -----------------------------------------------------

static bool ufs_name_eq(const char* stored, uint8_t len, const char* lookup) {
    for (uint8_t i = 0; i < len; ++i) {
        if (stored[i] != lookup[i]) return false;
    }
    return lookup[len] == '\0';
}

fk::core::Result<void, Error>
UfsFileSystem::list_dir_ino(uint32_t ino, uint64_t dir_size,
                             fk::containers::Vector<DirectoryEntry>& entries) {
    uint8_t* blk = static_cast<uint8_t*>(kmalloc(m_info.bsize));
    if (!blk) return Error::OutOfMemory;

    uint64_t processed = 0;
    while (processed < dir_size) {
        uint32_t lblock = static_cast<uint32_t>(processed / m_info.bsize);

        fk::core::Result<uint64_t, Error> phys_res =
            m_info.is_ufs2 ? get_block_frag2(ino, lblock)
                           : get_block_frag(ino, lblock);
        if (phys_res.is_error()) { kfree(blk); return phys_res.error(); }
        uint64_t phys_byte = phys_res.value();

        if (phys_byte == 0) { processed += m_info.bsize; continue; }

        if (m_device->read(phys_byte, m_info.bsize, blk).is_error()) {
            kfree(blk);
            return Error::IOError;
        }

        uint32_t off = 0;
        while (off + sizeof(UfsDirEntry) <= m_info.bsize) {
            const UfsDirEntry* de = reinterpret_cast<const UfsDirEntry*>(blk + off);
            if (de->d_reclen == 0) break;
            if (de->d_ino != 0 && de->d_namlen > 0) {
                const char* dname = reinterpret_cast<const char*>(blk + off + sizeof(UfsDirEntry));
                bool is_dot  = de->d_namlen == 1 && dname[0] == '.';
                bool is_ddot = de->d_namlen == 2 && dname[0] == '.' && dname[1] == '.';
                if (!is_dot && !is_ddot) {
                    DirectoryEntry ent;
                    size_t nlen = de->d_namlen < 255u ? de->d_namlen : 255u;
                    fk::memory::copy(ent.name, dname, nlen);
                    ent.name[nlen] = '\0';
                    ent.type = (de->d_type == UFS_DT_DIR) ? 1u : 0u;
                    entries.push_back(ent);
                }
            }
            off += de->d_reclen;
        }
        processed += m_info.bsize;
    }
    kfree(blk);
    return {};
}

fk::core::Result<fk::RefPtr<Node>, Error>
UfsFileSystem::lookup_in_ino(uint32_t ino, uint64_t dir_size, const char* name) {
    uint8_t* blk = static_cast<uint8_t*>(kmalloc(m_info.bsize));
    if (!blk) return Error::OutOfMemory;

    uint64_t processed = 0;
    while (processed < dir_size) {
        uint32_t lblock = static_cast<uint32_t>(processed / m_info.bsize);

        fk::core::Result<uint64_t, Error> phys_res =
            m_info.is_ufs2 ? get_block_frag2(ino, lblock)
                           : get_block_frag(ino, lblock);
        if (phys_res.is_error()) { kfree(blk); return phys_res.error(); }
        uint64_t phys_byte = phys_res.value();

        if (phys_byte == 0) { processed += m_info.bsize; continue; }

        if (m_device->read(phys_byte, m_info.bsize, blk).is_error()) {
            kfree(blk);
            return Error::IOError;
        }

        uint32_t off = 0;
        while (off + sizeof(UfsDirEntry) <= m_info.bsize) {
            const UfsDirEntry* de = reinterpret_cast<const UfsDirEntry*>(blk + off);
            if (de->d_reclen == 0) break;
            if (de->d_ino != 0 && de->d_namlen > 0) {
                const char* dname = reinterpret_cast<const char*>(blk + off + sizeof(UfsDirEntry));
                if (ufs_name_eq(dname, de->d_namlen, name)) {
                    uint32_t child_ino = de->d_ino;
                    kfree(blk);

                    uint64_t child_size;
                    uint16_t child_mode;
                    if (m_info.is_ufs2) {
                        Ufs2Dinode cdino;
                        if (read_inode2(child_ino, cdino).is_error())
                            return Error::IOError;
                        child_size = cdino.di_size;
                        child_mode = cdino.di_mode;
                    } else {
                        Ufs1Dinode cdino;
                        if (read_inode1(child_ino, cdino).is_error())
                            return Error::IOError;
                        child_size = cdino.di_size;
                        child_mode = cdino.di_mode;
                    }

                    auto node = fk::adopt_ref(new UfsNode(
                        fk::RefPtr<UfsFileSystem>(this), child_ino, child_size, child_mode));
                    if (!node) return Error::OutOfMemory;
                    return fk::RefPtr<Node>(node.ptr());
                }
            }
            off += de->d_reclen;
        }
        processed += m_info.bsize;
    }
    kfree(blk);
    return Error::NotFound;
}

// ---- Root Node interface -----------------------------------------------------

fk::core::Result<size_t, Error>
UfsFileSystem::read(uint64_t, size_t, uint8_t*) {
    return Error::IsDirectory;
}

fk::core::Result<size_t, Error>
UfsFileSystem::write(uint64_t, size_t, const uint8_t*) {
    return Error::IsDirectory;
}

fk::core::Result<fk::RefPtr<Node>, Error>
UfsFileSystem::lookup(const char* name) {
    uint64_t root_size;
    if (m_info.is_ufs2) {
        Ufs2Dinode dino;
        TRY(read_inode2(UFS_ROOT_INO, dino));
        root_size = dino.di_size;
    } else {
        Ufs1Dinode dino;
        TRY(read_inode1(UFS_ROOT_INO, dino));
        root_size = dino.di_size;
    }
    return lookup_in_ino(UFS_ROOT_INO, root_size, name);
}

fk::core::Result<void, Error>
UfsFileSystem::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    uint64_t root_size;
    if (m_info.is_ufs2) {
        Ufs2Dinode dino;
        TRY(read_inode2(UFS_ROOT_INO, dino));
        root_size = dino.di_size;
    } else {
        Ufs1Dinode dino;
        TRY(read_inode1(UFS_ROOT_INO, dino));
        root_size = dino.di_size;
    }
    return list_dir_ino(UFS_ROOT_INO, root_size, entries);
}

} // namespace fkernel
