#include <Kernel/Fs/Disk/Iso9660/iso9660_fs.h>
#include <Kernel/Fs/Disk/Iso9660/iso9660_node.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>
#include <LibFK/Algorithms/Generic/byte_order.h>
#include <LibFK/Algorithms/Generic/scatter_io.h>

namespace fkernel {

using fk::core::Error;

// ---- Sector I/O ---------------------------------------------------------------

fk::core::Result<void, Error>
Iso9660FileSystem::read_sector(uint32_t lba, uint8_t* buf) {
    auto res = m_device->read(static_cast<uint64_t>(lba) * ISO_SECTOR_SIZE,
                              ISO_SECTOR_SIZE, buf);
    if (res.is_error()) return Error::IOError;
    return {};
}

fk::core::Result<size_t, Error>
Iso9660FileSystem::read_bytes(uint32_t lba, uint64_t byte_offset, size_t size, uint8_t* buf) {
    if (size == 0) return static_cast<size_t>(0);
    uint8_t* sec_buf = static_cast<uint8_t*>(kmalloc(ISO_SECTOR_SIZE));
    if (!sec_buf) return Error::OutOfMemory;

    uint64_t offset = static_cast<uint64_t>(lba) * ISO_SECTOR_SIZE + byte_offset;
    auto resolve_block = [](uint32_t lblock) -> fk::core::Result<uint64_t, Error> {
        return static_cast<uint64_t>(lblock);
    };
    auto read_sector_cb = [this](uint64_t phys, uint8_t* scratch) -> fk::core::Result<void, Error> {
        return read_sector(static_cast<uint32_t>(phys), scratch);
    };

    auto res = fk::algorithms::scatter_read(offset, size, buf, ISO_SECTOR_SIZE, sec_buf,
                                            resolve_block, read_sector_cb);
    kfree(sec_buf);
    if (res.is_error()) return res.error();
    return res.value();
}

// ---- Static helpers -----------------------------------------------------------

void Iso9660FileSystem::strip_version(const char* src, uint8_t len, char* out) {
    uint8_t out_len = len;
    for (uint8_t i = 0; i < len; i++) {
        if (src[i] == ';') { out_len = i; break; }
    }
    if (out_len > 0 && src[out_len - 1] == '.') out_len--;
    fk::memory::copy(out, src, out_len);
    out[out_len] = '\0';
}

void Iso9660FileSystem::joliet_to_ascii(const uint8_t* ucs2, uint8_t len_bytes, char* out) {
    uint8_t n = len_bytes / 2;
    uint8_t out_pos = 0;

    // Locate version suffix ';' (UCS-2 BE 0x00 0x3B)
    uint8_t version_pos = n;
    for (uint8_t i = 0; i < n; i++) {
        if (ucs2[i * 2] == 0x00 && ucs2[i * 2 + 1] == 0x3B) {
            version_pos = i;
            break;
        }
    }

    for (uint8_t i = 0; i < version_pos; i++) {
        uint8_t hi = ucs2[i * 2];
        uint8_t lo = ucs2[i * 2 + 1];
        out[out_pos++] = (hi == 0x00 && lo < 0x80) ? static_cast<char>(lo) : '?';
    }
    if (out_pos > 0 && out[out_pos - 1] == '.') out_pos--;
    out[out_pos] = '\0';
}

bool Iso9660FileSystem::rr_get_nm(const uint8_t* su, uint8_t su_len,
                                   char* out, size_t out_max) {
    size_t out_pos = 0;
    bool found = false;
    uint8_t pos = 0;

    while (pos + 4 <= su_len) {
        uint8_t elen = su[pos + 2];
        if (elen < 4 || pos + elen > su_len) break;

        if (su[pos] == 'N' && su[pos + 1] == 'M' && elen >= 5) {
            uint8_t nm_flags = su[pos + 4];
            if (nm_flags & (NM_CURRENT | NM_PARENT)) { pos += elen; continue; }

            size_t nlen = elen - 5;
            size_t copy = (nlen < out_max - out_pos - 1) ? nlen : out_max - out_pos - 1;
            fk::memory::copy(out + out_pos, su + pos + 5, copy);
            out_pos += copy;
            found = true;

            if (!(nm_flags & NM_CONTINUE)) break;
        }
        pos += elen;
    }

    if (found) out[out_pos] = '\0';
    return found;
}

bool Iso9660FileSystem::rr_get_sl(const uint8_t* su, uint8_t su_len,
                                   char* out, size_t out_max) {
    size_t out_pos = 0;
    bool found = false;
    uint8_t pos = 0;

    while (pos + 5 <= su_len) {
        uint8_t elen = su[pos + 2];
        if (elen < 5 || pos + elen > su_len) { pos += (elen < 4 ? 4 : elen); continue; }

        if (su[pos] != 'S' || su[pos + 1] != 'L') { pos += elen; continue; }

        found = true;
        bool sl_cont = (su[pos + 4] & 0x01) != 0;

        uint8_t cp = 5;
        while (cp + 2 <= elen) {
            uint8_t cf = su[pos + cp];
            uint8_t cl = su[pos + cp + 1];

            if (cf & 0x08) { // ROOT
                if (out_pos < out_max - 1) out[out_pos++] = '/';
            } else if (cf & 0x02) { // CURRENT (.)
                if (out_pos < out_max - 2) out[out_pos++] = '.';
            } else if (cf & 0x04) { // PARENT (..)
                if (out_pos + 2 < out_max - 1) { out[out_pos++] = '.'; out[out_pos++] = '.'; }
            } else if (cl > 0) {
                if (out_pos > 0 && out[out_pos - 1] != '/') {
                    if (out_pos < out_max - 1) out[out_pos++] = '/';
                }
                size_t copy = (cl < out_max - out_pos - 1) ? cl : out_max - out_pos - 1;
                fk::memory::copy(out + out_pos, su + pos + cp + 2, copy);
                out_pos += copy;
            }
            cp += 2 + cl;
        }

        pos += elen;
        if (!sl_cont) break;
    }

    if (out_pos > 1 && out[out_pos - 1] == '/') out_pos--;
    out[out_pos] = '\0';
    return found;
}

// ---- parse_vd ----------------------------------------------------------------

bool Iso9660FileSystem::parse_vd(const uint8_t* vd, bool& got_pvd) {
    if (vd[1] != 'C' || vd[2] != 'D' || vd[3] != '0' || vd[4] != '0' || vd[5] != '1')
        return false;

    uint8_t type = vd[0];

    if (type == ISO_VD_TERM) return false;

    if (type == ISO_VD_PVD) {
        // Root directory record at PVD[156]; extent LBA at dr[2] (LE), size at dr[10] (LE)
        m_root_lba  = fk::algorithms::load_le32(vd, 158); // 156 + 2
        m_root_size = fk::algorithms::load_le32(vd, 166); // 156 + 10
        got_pvd = true;
    }

    if (type == ISO_VD_SVD) {
        // Joliet SVD: check escape sequences at offset 88
        const uint8_t* esc = vd + 88;
        bool is_joliet = (esc[0] == JOLIET_ESC0 && esc[1] == JOLIET_ESC1 &&
                          (esc[2] == '@' || esc[2] == 'C' || esc[2] == 'E'));
        if (is_joliet) {
            m_joliet           = true;
            m_joliet_root_lba  = fk::algorithms::load_le32(vd, 158);
            m_joliet_root_size = fk::algorithms::load_le32(vd, 166);
        }
    }

    return true;
}

// ---- create() factory --------------------------------------------------------

fk::core::Result<fk::RefPtr<Iso9660FileSystem>, Error>
Iso9660FileSystem::create(fk::RefPtr<StorageDevice> device) {
    uint8_t* sec = static_cast<uint8_t*>(kmalloc(ISO_SECTOR_SIZE));
    if (!sec) return Error::OutOfMemory;

    auto fs = fk::adopt_ref(new Iso9660FileSystem(device));
    if (!fs) { kfree(sec); return Error::OutOfMemory; }

    bool got_pvd = false;

    for (uint32_t lba = ISO_PVD_SECTOR; lba < ISO_PVD_SECTOR + 32; lba++) {
        if (fs->m_device->read(static_cast<uint64_t>(lba) * ISO_SECTOR_SIZE,
                               ISO_SECTOR_SIZE, sec).is_error())
            break;
        if (!fs->parse_vd(sec, got_pvd)) break;
    }

    if (!got_pvd) {
        kfree(sec);
        return Error::InvalidData;
    }

    // Detect Rock Ridge: look for SUSP SP in "." entry of root dir
    uint32_t rr_check_lba = fs->m_root_lba;
    if (fs->m_device->read(static_cast<uint64_t>(rr_check_lba) * ISO_SECTOR_SIZE,
                            ISO_SECTOR_SIZE, sec).is_ok()) {
        uint8_t dr_len     = sec[0];
        uint8_t file_id_len = sec[32];
        uint8_t su_off = 33 + file_id_len;
        if (file_id_len % 2 == 0) su_off++;
        uint8_t su_len = (su_off < dr_len) ? dr_len - su_off : 0;

        uint8_t p = 0;
        while (p + 4 <= su_len) {
            uint8_t elen = sec[su_off + p + 2];
            if (elen < 4 || p + elen > su_len) break;
            if (sec[su_off + p] == 'S' && sec[su_off + p + 1] == 'P' &&
                elen >= 7 &&
                sec[su_off + p + 5] == 0xBE && sec[su_off + p + 6] == 0xEF) {
                fs->m_rock_ridge = true;
                break;
            }
            p += elen;
        }
    }

    kfree(sec);
    fk::algorithms::klog("ISO9660", "Mounted: joliet=%d rock_ridge=%d root_lba=%u",
                         (int)fs->m_joliet, (int)fs->m_rock_ridge, fs->m_root_lba);
    return fs;
}

// ---- make_node_from_dr -------------------------------------------------------

fk::RefPtr<Node> Iso9660FileSystem::make_node_from_dr(const uint8_t* dr) {
    uint8_t dr_len      = dr[0];
    uint32_t extent_lba  = fk::algorithms::load_le32(dr, 2);
    uint32_t extent_size = fk::algorithms::load_le32(dr, 10);
    uint8_t  flags       = dr[25];
    uint8_t  file_id_len = dr[32];
    bool     is_dir      = (flags & ISO_FLAG_DIR) != 0;

    bool is_sym = false;
    fk::text::String sl_target;

    uint8_t su_off = 33 + file_id_len;
    if (file_id_len % 2 == 0) su_off++;
    uint8_t su_len = (su_off < dr_len) ? dr_len - su_off : 0;

    if (m_rock_ridge && su_len >= 5) {
        char sl_buf[256] = {};
        if (rr_get_sl(dr + su_off, su_len, sl_buf, sizeof(sl_buf))) {
            is_sym = true;
            is_dir = false;
            sl_target = fk::text::String(sl_buf);
        }
    }

    return fk::adopt_ref(new Iso9660Node(
        fk::RefPtr<Iso9660FileSystem>(this),
        extent_lba, extent_size, is_dir, is_sym, fk::types::move(sl_target)
    ));
}

// ---- Directory iteration helpers --------------------------------------------

static void iso_get_name(Iso9660FileSystem* fs, const uint8_t* dr,
                          uint8_t file_id_len, const uint8_t* su, uint8_t su_len,
                          char* out) {
    bool got = false;
    if (fs->m_rock_ridge && su_len >= 5)
        got = fs->rr_get_nm(su, su_len, out, 256);
    if (!got && fs->m_joliet)
        fs->joliet_to_ascii(dr + 33, file_id_len, out);
    if (!got && !fs->m_joliet)
        Iso9660FileSystem::strip_version(reinterpret_cast<const char*>(dr + 33),
                                          file_id_len, out);
}

// ---- list_dir_lba ------------------------------------------------------------

fk::core::Result<void, Error>
Iso9660FileSystem::list_dir_lba(uint32_t lba, uint32_t data_size,
                                 fk::containers::Vector<DirectoryEntry>& entries) {
    uint8_t* sec = static_cast<uint8_t*>(kmalloc(ISO_SECTOR_SIZE));
    if (!sec) return Error::OutOfMemory;

    uint32_t remaining = data_size;
    uint32_t cur_lba   = lba;

    while (remaining > 0) {
        if (m_device->read(static_cast<uint64_t>(cur_lba) * ISO_SECTOR_SIZE,
                           ISO_SECTOR_SIZE, sec).is_error()) {
            kfree(sec);
            return Error::IOError;
        }

        uint32_t pos = 0;
        while (pos < ISO_SECTOR_SIZE) {
            uint8_t dr_len = sec[pos];
            if (dr_len == 0) break;
            if (pos + dr_len > ISO_SECTOR_SIZE) break;

            uint8_t file_id_len = sec[pos + 32];

            // Skip "." (0x00) and ".." (0x01)
            if (file_id_len == 1 && (sec[pos + 33] == 0x00 || sec[pos + 33] == 0x01)) {
                pos += dr_len;
                continue;
            }

            uint8_t su_off = 33 + file_id_len;
            if (file_id_len % 2 == 0) su_off++;
            uint8_t su_len = (su_off < dr_len) ? dr_len - su_off : 0;
            const uint8_t* su = sec + pos + su_off;

            char name[256] = {};
            iso_get_name(this, sec + pos, file_id_len, su, su_len, name);

            uint8_t flags = sec[pos + 25];
            bool is_sym = false;
            if (m_rock_ridge && su_len >= 5) {
                char sl_buf[256] = {};
                is_sym = rr_get_sl(su, su_len, sl_buf, sizeof(sl_buf));
            }

            DirectoryEntry de{};
            size_t nlen = fk::memory::length(name);
            fk::memory::copy(de.name, name, nlen + 1);
            de.type = (flags & ISO_FLAG_DIR) ? 1u : (is_sym ? 2u : 0u);
            entries.push_back(de);

            pos += dr_len;
        }

        cur_lba++;
        remaining = (remaining >= ISO_SECTOR_SIZE) ? remaining - ISO_SECTOR_SIZE : 0;
    }

    kfree(sec);
    return {};
}

// ---- lookup_lba --------------------------------------------------------------

static bool iso_name_match(const char* entry, const char* lookup, bool case_insensitive) {
    size_t el = fk::memory::length(entry);
    size_t ll = fk::memory::length(lookup);
    if (el != ll) return false;
    if (!case_insensitive)
        return fk::memory::compare(entry, lookup, el) == 0;

    for (size_t i = 0; i < el; i++) {
        char a = entry[i], b = lookup[i];
        if (a >= 'a' && a <= 'z') a = static_cast<char>(a - 32);
        if (b >= 'a' && b <= 'z') b = static_cast<char>(b - 32);
        if (a != b) return false;
    }
    return true;
}

fk::core::Result<fk::RefPtr<Node>, Error>
Iso9660FileSystem::lookup_lba(uint32_t lba, uint32_t data_size, const char* name) {
    uint8_t* sec = static_cast<uint8_t*>(kmalloc(ISO_SECTOR_SIZE));
    if (!sec) return Error::OutOfMemory;

    uint32_t remaining = data_size;
    uint32_t cur_lba   = lba;
    fk::RefPtr<Node> found;
    bool io_error = false;

    while (remaining > 0 && !found) {
        if (m_device->read(static_cast<uint64_t>(cur_lba) * ISO_SECTOR_SIZE,
                           ISO_SECTOR_SIZE, sec).is_error()) {
            io_error = true;
            break;
        }

        uint32_t pos = 0;
        while (pos < ISO_SECTOR_SIZE && !found) {
            uint8_t dr_len = sec[pos];
            if (dr_len == 0) break;
            if (pos + dr_len > ISO_SECTOR_SIZE) break;

            uint8_t file_id_len = sec[pos + 32];

            if (file_id_len == 1 && (sec[pos + 33] == 0x00 || sec[pos + 33] == 0x01)) {
                pos += dr_len;
                continue;
            }

            uint8_t su_off = 33 + file_id_len;
            if (file_id_len % 2 == 0) su_off++;
            uint8_t su_len = (su_off < dr_len) ? dr_len - su_off : 0;
            const uint8_t* su = sec + pos + su_off;

            char entry_name[256] = {};
            iso_get_name(this, sec + pos, file_id_len, su, su_len, entry_name);

            // Case-insensitive for plain ISO9660; case-sensitive for RR/Joliet
            bool ci = (!m_rock_ridge && !m_joliet);
            if (iso_name_match(entry_name, name, ci))
                found = make_node_from_dr(sec + pos);

            pos += dr_len;
        }

        cur_lba++;
        remaining = (remaining >= ISO_SECTOR_SIZE) ? remaining - ISO_SECTOR_SIZE : 0;
    }

    kfree(sec);
    if (io_error) return Error::IOError;
    if (!found)   return Error::NotFound;
    return found;
}

// ---- Root Node interface -----------------------------------------------------

fk::core::Result<size_t, Error>
Iso9660FileSystem::read(uint64_t /*offset*/, size_t /*size*/, uint8_t* /*buffer*/) {
    return Error::IsDirectory;
}

fk::core::Result<size_t, Error>
Iso9660FileSystem::write(uint64_t /*offset*/, size_t /*size*/, const uint8_t* /*buffer*/) {
    return Error::IsDirectory;
}

fk::core::Result<fk::RefPtr<Node>, Error>
Iso9660FileSystem::lookup(const char* name) {
    uint32_t root_lba  = m_joliet ? m_joliet_root_lba  : m_root_lba;
    uint32_t root_size = m_joliet ? m_joliet_root_size : m_root_size;
    return lookup_lba(root_lba, root_size, name);
}

fk::core::Result<void, Error>
Iso9660FileSystem::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    uint32_t root_lba  = m_joliet ? m_joliet_root_lba  : m_root_lba;
    uint32_t root_size = m_joliet ? m_joliet_root_size : m_root_size;
    return list_dir_lba(root_lba, root_size, entries);
}

} // namespace fkernel
