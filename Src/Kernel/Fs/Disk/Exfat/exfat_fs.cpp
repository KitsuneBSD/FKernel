#include <Kernel/Fs/Disk/Exfat/exfat_fs.h>
#include <Kernel/Fs/Disk/Exfat/exfat_node.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Memory/heap_malloc.h>

namespace fkernel {

using fk::core::Error;
using fk::core::Result;

// ---- Cluster address ---------------------------------------------------------

uint64_t ExfatFileSystem::cluster_to_byte(uint32_t cluster) const {
    return (static_cast<uint64_t>(m_heap_offset) +
            static_cast<uint64_t>(cluster - EXFAT_CLUSTER_FIRST) * m_sectors_per_cluster)
           * m_bytes_per_sector;
}

// ---- Cluster I/O -------------------------------------------------------------

Result<void, Error> ExfatFileSystem::read_cluster(uint32_t cluster, uint8_t* buf) {
    uint64_t off = cluster_to_byte(cluster);
    for (uint32_t s = 0; s < m_sectors_per_cluster; s++) {
        if (m_device->read(off + static_cast<uint64_t>(s) * m_bytes_per_sector,
                           m_bytes_per_sector, buf + s * m_bytes_per_sector).is_error())
            return Error::IOError;
    }
    return {};
}

Result<void, Error> ExfatFileSystem::write_cluster(uint32_t cluster, const uint8_t* buf) {
    uint64_t off = cluster_to_byte(cluster);
    for (uint32_t s = 0; s < m_sectors_per_cluster; s++) {
        if (m_device->write(off + static_cast<uint64_t>(s) * m_bytes_per_sector,
                            m_bytes_per_sector, buf + s * m_bytes_per_sector).is_error())
            return Error::IOError;
    }
    return {};
}

// ---- FAT helpers -------------------------------------------------------------

uint32_t ExfatFileSystem::next_fat(uint32_t cluster) {
    uint64_t off = static_cast<uint64_t>(m_fat_offset) + cluster * 4;
    uint32_t val = 0;
    m_device->read(off, 4, reinterpret_cast<uint8_t*>(&val));
    return val;
}

Result<void, Error> ExfatFileSystem::set_fat(uint32_t cluster, uint32_t value) {
    uint64_t off = static_cast<uint64_t>(m_fat_offset) + cluster * 4;
    if (m_device->write(off, 4, reinterpret_cast<const uint8_t*>(&value)).is_error())
        return Error::IOError;
    return {};
}

// ---- Bitmap helpers ----------------------------------------------------------

bool ExfatFileSystem::bitmap_get(uint32_t cluster) {
    uint32_t idx = cluster - EXFAT_CLUSTER_FIRST;
    uint64_t byte_off = cluster_to_byte(m_bitmap_cluster) + idx / 8;
    uint8_t byte = 0;
    m_device->read(byte_off, 1, &byte);
    return (byte >> (idx % 8)) & 1;
}

Result<void, Error> ExfatFileSystem::bitmap_set(uint32_t cluster, bool used) {
    uint32_t idx = cluster - EXFAT_CLUSTER_FIRST;
    uint64_t byte_off = cluster_to_byte(m_bitmap_cluster) + idx / 8;
    uint8_t byte = 0;
    if (m_device->read(byte_off, 1, &byte).is_error()) return Error::IOError;
    if (used)
        byte = static_cast<uint8_t>(byte | (1u << (idx % 8)));
    else
        byte = static_cast<uint8_t>(byte & ~(1u << (idx % 8)));
    if (m_device->write(byte_off, 1, &byte).is_error()) return Error::IOError;
    return {};
}

Result<uint32_t, Error> ExfatFileSystem::alloc_cluster(uint32_t prev) {
    for (uint32_t c = EXFAT_CLUSTER_FIRST; c < EXFAT_CLUSTER_FIRST + m_cluster_count; c++) {
        if (!bitmap_get(c)) {
            TRY(bitmap_set(c, true));
            TRY(set_fat(c, EXFAT_CLUSTER_EOC));
            if (prev >= EXFAT_CLUSTER_FIRST) TRY(set_fat(prev, c));
            return c;
        }
    }
    return Error::NoSpaceLeftOnDevice;
}

void ExfatFileSystem::free_chain(uint32_t first_cluster) {
    uint32_t c = first_cluster;
    while (c >= EXFAT_CLUSTER_FIRST && c < EXFAT_CLUSTER_EOC) {
        uint32_t next = next_fat(c);
        set_fat(c, EXFAT_CLUSTER_FREE);
        bitmap_set(c, false);
        c = next;
    }
}

// ---- Cluster chain I/O -------------------------------------------------------

Result<size_t, Error>
ExfatFileSystem::read_chain(uint32_t first_cluster, bool no_fat_chain,
                             uint64_t data_length, uint64_t offset,
                             size_t size, uint8_t* buf) {
    if (offset >= data_length) return static_cast<size_t>(0);
    uint64_t avail = data_length - offset;
    if (size > static_cast<size_t>(avail)) size = static_cast<size_t>(avail);

    uint8_t* cbuf = static_cast<uint8_t*>(kmalloc(m_bytes_per_cluster));
    if (!cbuf) return Error::OutOfMemory;

    uint32_t cluster = first_cluster;
    uint64_t cluster_idx = offset / m_bytes_per_cluster;

    // Skip to the starting cluster
    for (uint64_t i = 0; i < cluster_idx; i++) {
        cluster = no_fat_chain ? cluster + 1 : next_fat(cluster);
        if (cluster < EXFAT_CLUSTER_FIRST || cluster >= EXFAT_CLUSTER_EOC) {
            kfree(cbuf);
            return Error::IOError;
        }
    }

    size_t done = 0;
    bool ok = true;
    while (done < size && ok) {
        if (read_cluster(cluster, cbuf).is_error()) { ok = false; break; }
        uint64_t in_cluster = (offset + done) % m_bytes_per_cluster;
        size_t chunk = static_cast<size_t>(m_bytes_per_cluster - in_cluster);
        if (chunk > size - done) chunk = size - done;
        fk::memory::copy(buf + done, cbuf + in_cluster, chunk);
        done += chunk;
        if (done < size) {
            cluster = no_fat_chain ? cluster + 1 : next_fat(cluster);
            if (cluster < EXFAT_CLUSTER_FIRST || cluster >= EXFAT_CLUSTER_EOC) ok = false;
        }
    }

    kfree(cbuf);
    if (!ok) return Error::IOError;
    return done;
}

Result<size_t, Error>
ExfatFileSystem::write_chain(uint32_t& first_cluster, bool no_fat_chain,
                              uint64_t& data_length, uint64_t offset,
                              size_t size, const uint8_t* buf) {
    uint8_t* cbuf = static_cast<uint8_t*>(kmalloc(m_bytes_per_cluster));
    if (!cbuf) return Error::OutOfMemory;

    uint32_t cluster = first_cluster;
    uint64_t cluster_idx = offset / m_bytes_per_cluster;
    uint64_t last_cluster_idx = (offset + size - 1) / m_bytes_per_cluster;

    // Allocate clusters if needed
    uint64_t needed_clusters = last_cluster_idx + 1;
    uint64_t have_clusters = (data_length + m_bytes_per_cluster - 1) / m_bytes_per_cluster;

    if (cluster == EXFAT_CLUSTER_FIRST - 1 || cluster == 0) {
        // No clusters yet
        auto res = alloc_cluster(EXFAT_CLUSTER_FIRST - 1);
        if (res.is_error()) { kfree(cbuf); return res.error(); }
        first_cluster = res.value();
        cluster = first_cluster;
        have_clusters = 1;
    }

    // Walk to last existing cluster to extend if needed
    uint32_t tail = cluster;
    for (uint64_t i = 1; i < have_clusters && !no_fat_chain; i++) {
        uint32_t n = next_fat(tail);
        if (n < EXFAT_CLUSTER_FIRST || n >= EXFAT_CLUSTER_EOC) break;
        tail = n;
    }

    while (have_clusters < needed_clusters) {
        auto res = alloc_cluster(tail);
        if (res.is_error()) { kfree(cbuf); return res.error(); }
        tail = res.value();
        have_clusters++;
    }

    // Walk to starting cluster
    cluster = first_cluster;
    for (uint64_t i = 0; i < cluster_idx; i++) {
        cluster = no_fat_chain ? cluster + 1 : next_fat(cluster);
    }

    size_t done = 0;
    bool ok = true;
    while (done < size && ok) {
        if (read_cluster(cluster, cbuf).is_error()) { ok = false; break; }
        uint64_t in_cluster = (offset + done) % m_bytes_per_cluster;
        size_t chunk = static_cast<size_t>(m_bytes_per_cluster - in_cluster);
        if (chunk > size - done) chunk = size - done;
        fk::memory::copy(cbuf + in_cluster, buf + done, chunk);
        if (write_cluster(cluster, cbuf).is_error()) { ok = false; break; }
        done += chunk;
        if (done < size) {
            cluster = no_fat_chain ? cluster + 1 : next_fat(cluster);
            if (cluster < EXFAT_CLUSTER_FIRST || cluster >= EXFAT_CLUSTER_EOC) ok = false;
        }
    }

    uint64_t new_end = offset + done;
    if (new_end > data_length) data_length = new_end;

    kfree(cbuf);
    if (!ok) return Error::IOError;
    return done;
}

// ---- UCS-2 helpers (file-local) ----------------------------------------------

static void ucs2_to_ascii(const uint16_t* ucs2, uint8_t len, char* out) {
    for (uint8_t i = 0; i < len; i++) {
        uint16_t c = ucs2[i];
        out[i] = (c < 0x80) ? static_cast<char>(c) : '?';
    }
    out[len] = '\0';
}

static void ascii_to_ucs2(const char* str, uint8_t len, uint16_t* ucs2) {
    for (uint8_t i = 0; i < len; i++)
        ucs2[i] = static_cast<uint16_t>(static_cast<uint8_t>(str[i]));
}

static bool exfat_name_equal(const char* a, const char* b) {
    size_t la = fk::memory::length(a);
    size_t lb = fk::memory::length(b);
    if (la != lb) return false;
    for (size_t i = 0; i < la; i++) {
        char ca = a[i], cb = b[i];
        if (ca >= 'a' && ca <= 'z') ca = static_cast<char>(ca - 32);
        if (cb >= 'a' && cb <= 'z') cb = static_cast<char>(cb - 32);
        if (ca != cb) return false;
    }
    return true;
}

// Helper struct for a parsed directory entry set
struct ExfatDirSet {
    bool     valid{false};
    bool     is_dir{false};
    bool     no_fat_chain{false};
    uint8_t  name_length{0};
    uint32_t first_cluster{0};
    uint64_t data_length{0};
    char     name[256]{};
    uint32_t file_entry_offset{0}; // byte offset within cluster chain
};

static ExfatDirSet parse_entry_set(const uint8_t* fe, const uint8_t* se, const uint8_t* ne,
                                    uint8_t secondary_count, uint32_t file_entry_offset) {
    ExfatDirSet ds;
    if (fe[0] != EXFAT_ET_FILE || se[0] != EXFAT_ET_STREAM_EXT) return ds;

    const ExfatFileEntry*     f  = reinterpret_cast<const ExfatFileEntry*>(fe);
    const ExfatStreamExtEntry* s = reinterpret_cast<const ExfatStreamExtEntry*>(se);

    ds.is_dir       = (f->file_attributes & EXFAT_ATTR_DIRECTORY) != 0;
    ds.no_fat_chain = (s->general_secondary_flags & EXFAT_SFLAG_NO_FAT_CHAIN) != 0;
    ds.name_length  = s->name_length;
    ds.first_cluster = s->first_cluster;
    ds.data_length  = s->data_length;
    ds.file_entry_offset = file_entry_offset;

    // Collect name from File Name entries (each holds 15 UCS-2 chars)
    uint8_t name_pos = 0;
    uint8_t fname_count = secondary_count - 1; // secondary_count includes stream ext
    for (uint8_t i = 0; i < fname_count && name_pos < ds.name_length; i++) {
        const ExfatFileNameEntry* fn =
            reinterpret_cast<const ExfatFileNameEntry*>(ne + i * 32);
        if (fn->entry_type != EXFAT_ET_FILE_NAME) break;
        uint8_t chars = static_cast<uint8_t>(ds.name_length - name_pos);
        if (chars > 15) chars = 15;
        ucs2_to_ascii(fn->file_name, chars, ds.name + name_pos);
        name_pos = static_cast<uint8_t>(name_pos + chars);
    }
    ds.name[ds.name_length] = '\0';
    ds.valid = true;
    return ds;
}

// ---- Directory iteration core ------------------------------------------------

// Walks one cluster chain's directory entries, calling callback for each complete set.
// Returns the ExfatDirSet for the matching name (for lookup) or all entries (for list).

struct DirWalkResult {
    enum Kind { LIST, FOUND, NOT_FOUND, IO_ERROR };
    Kind kind{NOT_FOUND};
    ExfatDirSet found_set;
};

static constexpr uint32_t EXFAT_MAX_DIR_ENTRIES = 4096;
static constexpr uint8_t  EXFAT_MAX_SECONDARY   = 18;  // 1 stream + 17 name entries

static DirWalkResult walk_dir_cluster(ExfatFileSystem* fs, uint32_t first_cluster,
                                       const char* lookup_name,
                                       fk::containers::Vector<DirectoryEntry>* out_list) {
    DirWalkResult result;
    result.kind = DirWalkResult::NOT_FOUND;

    uint8_t* cbuf = static_cast<uint8_t*>(kmalloc(fs->m_bytes_per_cluster));
    if (!cbuf) { result.kind = DirWalkResult::IO_ERROR; return result; }

    // Secondary entries buffer: up to EXFAT_MAX_SECONDARY × 32 bytes
    uint8_t sec_buf[EXFAT_MAX_SECONDARY * 32] = {};

    uint32_t cluster = first_cluster;
    uint32_t chain_offset = 0;  // byte offset from start of cluster chain
    uint32_t entry_count = 0;

    while (cluster >= EXFAT_CLUSTER_FIRST && cluster < EXFAT_CLUSTER_EOC
           && entry_count < EXFAT_MAX_DIR_ENTRIES) {
        if (fs->read_cluster(cluster, cbuf).is_error()) {
            result.kind = DirWalkResult::IO_ERROR;
            break;
        }

        uint32_t pos = 0;
        while (pos + 32 <= fs->m_bytes_per_cluster) {
            uint8_t type = cbuf[pos];

            if (type == EXFAT_ET_END_OF_DIR) goto done;
            if (!(type & EXFAT_INUSE_BIT)) { pos += 32; continue; }

            if (type == EXFAT_ET_FILE) {
                uint8_t sec_count = cbuf[pos + 1];
                if (sec_count < 2 || sec_count > EXFAT_MAX_SECONDARY) {
                    pos += 32; continue;
                }

                uint32_t file_offset = chain_offset + pos;

                // Collect secondary entries
                fk::memory::set(sec_buf, 0, sizeof(sec_buf));
                uint32_t spos = pos + 32;
                bool ok = true;
                for (uint8_t i = 0; i < sec_count; i++) {
                    if (spos >= fs->m_bytes_per_cluster) { ok = false; break; }
                    fk::memory::copy(sec_buf + i * 32, cbuf + spos, 32);
                    spos += 32;
                }
                if (!ok) { pos = spos; continue; }

                ExfatDirSet ds = parse_entry_set(
                    cbuf + pos, sec_buf, sec_buf + 32,
                    sec_count, file_offset);

                if (!ds.valid) { pos += 32 + sec_count * 32; continue; }

                if (lookup_name) {
                    if (exfat_name_equal(ds.name, lookup_name)) {
                        result.kind = DirWalkResult::FOUND;
                        result.found_set = ds;
                        goto done;
                    }
                } else if (out_list) {
                    DirectoryEntry de{};
                    size_t nlen = fk::memory::length(ds.name);
                    fk::memory::copy(de.name, ds.name, nlen + 1);
                    de.type = ds.is_dir ? 1u : 0u;
                    out_list->push_back(de);
                    entry_count++;
                }

                pos += 32 + static_cast<uint32_t>(sec_count) * 32;
                continue;
            }

            pos += 32;
        }

        chain_offset += fs->m_bytes_per_cluster;
        cluster = fs->next_fat(cluster);
    }

done:
    kfree(cbuf);
    return result;
}

// ---- list_dir_cluster / lookup_cluster ----------------------------------------

Result<void, Error>
ExfatFileSystem::list_dir_cluster(uint32_t first_cluster,
                                   fk::containers::Vector<DirectoryEntry>& entries) {
    auto r = walk_dir_cluster(this, first_cluster, nullptr, &entries);
    if (r.kind == DirWalkResult::IO_ERROR) return Error::IOError;
    return {};
}

Result<fk::RefPtr<Node>, Error>
ExfatFileSystem::lookup_cluster(uint32_t first_cluster, const char* name) {
    auto r = walk_dir_cluster(this, first_cluster, name, nullptr);
    if (r.kind == DirWalkResult::IO_ERROR) return Error::IOError;
    if (r.kind != DirWalkResult::FOUND) return Error::NotFound;

    auto& ds = r.found_set;
    auto node = fk::adopt_ref(new ExfatNode(
        fk::RefPtr<ExfatFileSystem>(this),
        ds.first_cluster, ds.data_length, ds.is_dir, ds.no_fat_chain,
        first_cluster, ds.file_entry_offset));
    return fk::RefPtr<Node>(node);
}

// ---- create_entry ------------------------------------------------------------

Result<fk::RefPtr<Node>, Error>
ExfatFileSystem::create_entry(uint32_t dir_cluster, const char* name, bool is_dir) {
    uint8_t name_len = static_cast<uint8_t>(fk::memory::length(name));
    if (name_len == 0 || name_len > 255) return Error::InvalidParameter;

    uint8_t fn_count = static_cast<uint8_t>((name_len + 14) / 15);
    uint8_t total_secondary = static_cast<uint8_t>(1 + fn_count); // 1 StreamExt + fn_count FileName

    uint8_t* cbuf = static_cast<uint8_t*>(kmalloc(m_bytes_per_cluster));
    if (!cbuf) return Error::OutOfMemory;

    // Find or allocate a slot with (1 + total_secondary) consecutive free entries
    uint32_t cluster = dir_cluster;
    uint32_t chain_offset = 0;
    uint32_t slot_cluster = 0, slot_pos = 0;
    uint8_t  consecutive = 0;

    while (cluster >= EXFAT_CLUSTER_FIRST && cluster < EXFAT_CLUSTER_EOC) {
        if (read_cluster(cluster, cbuf).is_error()) { kfree(cbuf); return Error::IOError; }

        for (uint32_t p = 0; p + 32 <= m_bytes_per_cluster; p += 32) {
            uint8_t type = cbuf[p];
            bool free_slot = (type == EXFAT_ET_END_OF_DIR || !(type & EXFAT_INUSE_BIT));

            if (free_slot) {
                if (consecutive == 0) { slot_cluster = cluster; slot_pos = p; }
                consecutive++;
                if (consecutive == static_cast<uint8_t>(1 + total_secondary)) goto found_slot;
            } else {
                consecutive = 0;
            }
            if (type == EXFAT_ET_END_OF_DIR) break;
        }

        chain_offset += m_bytes_per_cluster;
        uint32_t next = next_fat(cluster);
        if (next < EXFAT_CLUSTER_FIRST || next >= EXFAT_CLUSTER_EOC) {
            // Extend directory
            auto res = alloc_cluster(cluster);
            if (res.is_error()) { kfree(cbuf); return res.error(); }
            uint8_t* zero = static_cast<uint8_t*>(kmalloc(m_bytes_per_cluster));
            if (zero) { fk::memory::set(zero, 0, m_bytes_per_cluster); write_cluster(res.value(), zero); kfree(zero); }
            next = res.value();
        }
        cluster = next;
    }

found_slot:
    if (consecutive < static_cast<uint8_t>(1 + total_secondary)) {
        kfree(cbuf);
        return Error::NoSpaceLeftOnDevice;
    }

    // Allocate data cluster(s) for the new entry
    uint32_t new_first_cluster = 0;
    uint64_t init_data_length = 0;
    if (is_dir) {
        auto res = alloc_cluster(0);
        if (res.is_error()) { kfree(cbuf); return res.error(); }
        new_first_cluster = res.value();
        init_data_length = m_bytes_per_cluster;
        // Zero out the new directory cluster
        fk::memory::set(cbuf, 0, m_bytes_per_cluster);
        write_cluster(new_first_cluster, cbuf);
    }

    // Re-read the slot cluster
    if (read_cluster(slot_cluster, cbuf).is_error()) { kfree(cbuf); return Error::IOError; }

    // Build File entry at slot_pos
    ExfatFileEntry* fe = reinterpret_cast<ExfatFileEntry*>(cbuf + slot_pos);
    fk::memory::set(fe, 0, 32);
    fe->entry_type      = EXFAT_ET_FILE;
    fe->secondary_count = total_secondary;
    fe->file_attributes = is_dir ? EXFAT_ATTR_DIRECTORY : EXFAT_ATTR_ARCHIVE;

    // Build Stream Extension at slot_pos + 32
    ExfatStreamExtEntry* se =
        reinterpret_cast<ExfatStreamExtEntry*>(cbuf + slot_pos + 32);
    fk::memory::set(se, 0, 32);
    se->entry_type = EXFAT_ET_STREAM_EXT;
    se->general_secondary_flags = EXFAT_SFLAG_ALLOC_POSSIBLE;
    se->name_length  = name_len;
    se->first_cluster = new_first_cluster;
    se->valid_data_length = init_data_length;
    se->data_length  = init_data_length;

    // Build File Name entries starting at slot_pos + 64
    uint8_t np = 0;
    for (uint8_t i = 0; i < fn_count; i++) {
        ExfatFileNameEntry* fn =
            reinterpret_cast<ExfatFileNameEntry*>(cbuf + slot_pos + 64 + i * 32);
        fk::memory::set(fn, 0, 32);
        fn->entry_type = EXFAT_ET_FILE_NAME;
        uint8_t chars = static_cast<uint8_t>(name_len - np);
        if (chars > 15) chars = 15;
        ascii_to_ucs2(name + np, chars, fn->file_name);
        np = static_cast<uint8_t>(np + chars);
    }

    if (write_cluster(slot_cluster, cbuf).is_error()) { kfree(cbuf); return Error::IOError; }

    kfree(cbuf);

    uint32_t entry_offset = chain_offset + slot_pos;
    auto node = fk::adopt_ref(new ExfatNode(
        fk::RefPtr<ExfatFileSystem>(this),
        new_first_cluster, init_data_length, is_dir, false,
        dir_cluster, entry_offset));
    return fk::RefPtr<Node>(node);
}

// ---- delete_entry ------------------------------------------------------------

Result<void, Error> ExfatFileSystem::delete_entry(uint32_t dir_cluster, const char* name) {
    auto r = walk_dir_cluster(this, dir_cluster, name, nullptr);
    if (r.kind == DirWalkResult::IO_ERROR) return Error::IOError;
    if (r.kind != DirWalkResult::FOUND) return Error::NotFound;

    auto& ds = r.found_set;

    // Find the cluster + offset of the File entry
    uint32_t cluster = dir_cluster;
    uint32_t chain_off = 0;
    while (chain_off + m_bytes_per_cluster <= ds.file_entry_offset) {
        chain_off += m_bytes_per_cluster;
        cluster = next_fat(cluster);
        if (cluster < EXFAT_CLUSTER_FIRST || cluster >= EXFAT_CLUSTER_EOC)
            return Error::IOError;
    }
    uint32_t in_cluster = ds.file_entry_offset - chain_off;

    uint8_t* cbuf = static_cast<uint8_t*>(kmalloc(m_bytes_per_cluster));
    if (!cbuf) return Error::OutOfMemory;

    if (read_cluster(cluster, cbuf).is_error()) { kfree(cbuf); return Error::IOError; }

    // How many entries to mark deleted
    uint8_t sec_count = cbuf[in_cluster + 1];
    uint8_t total_entries = static_cast<uint8_t>(1 + sec_count);

    for (uint8_t i = 0; i < total_entries; i++) {
        uint32_t ep = in_cluster + i * 32;
        if (ep + 32 > m_bytes_per_cluster) break;
        cbuf[ep] = static_cast<uint8_t>(cbuf[ep] & ~EXFAT_INUSE_BIT);
    }

    if (write_cluster(cluster, cbuf).is_error()) { kfree(cbuf); return Error::IOError; }
    kfree(cbuf);

    // Free cluster chain if it had data
    if (ds.first_cluster >= EXFAT_CLUSTER_FIRST)
        free_chain(ds.first_cluster);

    return {};
}

// ---- update_stream_ext -------------------------------------------------------

Result<void, Error>
ExfatFileSystem::update_stream_ext(uint32_t dir_cluster, uint32_t dir_offset,
                                    uint64_t new_size) {
    uint32_t cluster = dir_cluster;
    uint32_t chain_off = 0;
    while (chain_off + m_bytes_per_cluster <= dir_offset) {
        chain_off += m_bytes_per_cluster;
        cluster = next_fat(cluster);
        if (cluster < EXFAT_CLUSTER_FIRST || cluster >= EXFAT_CLUSTER_EOC)
            return Error::IOError;
    }
    uint32_t in_cluster = dir_offset - chain_off;

    uint8_t* cbuf = static_cast<uint8_t*>(kmalloc(m_bytes_per_cluster));
    if (!cbuf) return Error::OutOfMemory;

    if (read_cluster(cluster, cbuf).is_error()) { kfree(cbuf); return Error::IOError; }

    ExfatStreamExtEntry* se =
        reinterpret_cast<ExfatStreamExtEntry*>(cbuf + in_cluster + 32);
    if (se->entry_type == EXFAT_ET_STREAM_EXT) {
        se->valid_data_length = new_size;
        se->data_length       = new_size;
    }

    if (write_cluster(cluster, cbuf).is_error()) { kfree(cbuf); return Error::IOError; }
    kfree(cbuf);
    return {};
}

// ---- create() factory --------------------------------------------------------

Result<fk::RefPtr<ExfatFileSystem>, Error>
ExfatFileSystem::create(fk::RefPtr<StorageDevice> device) {
    ExfatBpb bpb;
    if (device->read(0, sizeof(bpb), reinterpret_cast<uint8_t*>(&bpb)).is_error())
        return Error::IOError;

    // Validate OEM name "EXFAT   " (8 bytes, 5 chars + 3 spaces)
    if (fk::memory::compare(bpb.oem_name, "EXFAT   ", 8) != 0)
        return Error::InvalidData;

    // Validate BytesPerSectorShift (9–12)
    if (bpb.bytes_per_sector_shift < 9 || bpb.bytes_per_sector_shift > 12)
        return Error::InvalidData;

    auto fs = fk::adopt_ref(new ExfatFileSystem(device));
    if (!fs) return Error::OutOfMemory;

    fs->m_bytes_per_sector   = 1u << bpb.bytes_per_sector_shift;
    fs->m_sectors_per_cluster = 1u << bpb.sectors_per_cluster_shift;
    fs->m_bytes_per_cluster  = fs->m_bytes_per_sector * fs->m_sectors_per_cluster;
    fs->m_fat_offset         = static_cast<uint64_t>(bpb.fat_offset) * fs->m_bytes_per_sector;
    fs->m_heap_offset        = bpb.cluster_heap_offset;
    fs->m_root_cluster       = bpb.root_cluster;
    fs->m_cluster_count      = bpb.cluster_count;
    fs->m_bitmap_cluster     = 0;
    fs->m_bitmap_size        = (bpb.cluster_count + 7) / 8;

    // Scan root directory to find allocation bitmap entry
    uint8_t* cbuf = static_cast<uint8_t*>(kmalloc(fs->m_bytes_per_cluster));
    if (!cbuf) return Error::OutOfMemory;

    uint32_t cluster = fs->m_root_cluster;
    while (cluster >= EXFAT_CLUSTER_FIRST && cluster < EXFAT_CLUSTER_EOC
           && fs->m_bitmap_cluster == 0) {
        if (fs->read_cluster(cluster, cbuf).is_error()) break;
        for (uint32_t p = 0; p + 32 <= fs->m_bytes_per_cluster; p += 32) {
            if (cbuf[p] == EXFAT_ET_ALLOC_BITMAP) {
                const ExfatAllocBitmapEntry* be =
                    reinterpret_cast<const ExfatAllocBitmapEntry*>(cbuf + p);
                if (!(be->bitmap_flags & 0x01)) { // primary bitmap
                    fs->m_bitmap_cluster = be->first_cluster;
                    break;
                }
            }
            if (cbuf[p] == EXFAT_ET_END_OF_DIR) goto bitmap_done;
        }
        cluster = fs->next_fat(cluster);
    }

bitmap_done:
    kfree(cbuf);
    if (fs->m_bitmap_cluster == 0) return Error::InvalidData;

    fk::algorithms::klog("EXFAT", "Mounted: bpc=%u root_cluster=%u cluster_count=%u",
                          fs->m_bytes_per_cluster, fs->m_root_cluster, fs->m_cluster_count);
    return fs;
}

// ---- Root Node interface -----------------------------------------------------

Result<size_t, Error>
ExfatFileSystem::read(uint64_t /*offset*/, size_t /*size*/, uint8_t* /*buffer*/) {
    return Error::IsDirectory;
}

Result<size_t, Error>
ExfatFileSystem::write(uint64_t /*offset*/, size_t /*size*/, const uint8_t* /*buffer*/) {
    return Error::IsDirectory;
}

Result<fk::RefPtr<Node>, Error> ExfatFileSystem::lookup(const char* name) {
    return lookup_cluster(m_root_cluster, name);
}

Result<void, Error>
ExfatFileSystem::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    return list_dir_cluster(m_root_cluster, entries);
}

Result<fk::RefPtr<Node>, Error>
ExfatFileSystem::create_child(const char* name, int /*mode*/) {
    return create_entry(m_root_cluster, name, false);
}

Result<fk::RefPtr<Node>, Error>
ExfatFileSystem::mkdir(const char* name, int /*mode*/) {
    return create_entry(m_root_cluster, name, true);
}

Result<void, Error> ExfatFileSystem::unlink(const char* name) {
    return delete_entry(m_root_cluster, name);
}

Result<void, Error> ExfatFileSystem::rmdir(const char* name) {
    return delete_entry(m_root_cluster, name);
}

} // namespace fkernel
