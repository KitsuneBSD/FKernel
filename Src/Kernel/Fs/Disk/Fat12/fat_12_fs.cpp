#include <Kernel/Fs/Disk/Fat12/fat_12_fs.h>
#include <Kernel/Fs/Disk/Fat12/fat_12_node.h>
#include <Kernel/Fs/Disk/Fat12/bpb.h>
#include <Kernel/Fs/Disk/Fat12/directory_entry.h>
#include <LibFK/Algorithms/Generic/fat_name.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<Fat12FileSystem>, fk::core::Error>
Fat12FileSystem::create(fk::RefPtr<StorageDevice> device) {
    Fat12Bpb bpb;
    if (device->read(0, sizeof(bpb), reinterpret_cast<uint8_t*>(&bpb)).is_error())
        return fk::core::Error::IOError;

    // Basic validation for FAT12
    if (bpb.bytes_per_sector != 512) return fk::core::Error::InvalidData;
    if (bpb.sectors_per_cluster == 0) return fk::core::Error::InvalidData;
    if (bpb.root_entry_count == 0) return fk::core::Error::InvalidData;
    if (bpb.fat_count == 0 || bpb.fat_count > 2) return fk::core::Error::InvalidData;
    if (bpb.fat_size_16 == 0) return fk::core::Error::InvalidData;
    
    // Check filesystem type signature
    if (fk::memory::compare(bpb.fs_type, "FAT12   ", 8) != 0) {
        return fk::core::Error::InvalidData;
    }
    
    // Validate it's FAT12
    uint32_t total_sectors = bpb.total_sectors_16 != 0 ? bpb.total_sectors_16 : bpb.total_sectors_32;
    if (total_sectors == 0) return fk::core::Error::InvalidData;
    
    uint32_t data_sectors = total_sectors - (bpb.reserved_sectors + (bpb.fat_count * bpb.fat_size_16) + ((bpb.root_entry_count * 32) / 512));
    if (data_sectors <= 0) return fk::core::Error::InvalidData;
    
    uint32_t clusters = data_sectors / bpb.sectors_per_cluster;
    if (clusters >= 4085) return fk::core::Error::InvalidData;

    auto fs = fk::adopt_ref(new Fat12FileSystem(device));
    if (!fs) return fk::core::Error::OutOfMemory;

    fs->m_fat_sector = bpb.reserved_sectors;
    fs->m_root_dir_sectors = (bpb.root_entry_count * 32) / 512;
    fs->m_first_data_sector = bpb.reserved_sectors + (bpb.fat_count * bpb.fat_size_16) + fs->m_root_dir_sectors;

    return fs;
}

uint32_t Fat12FileSystem::cluster_to_sector(uint32_t cluster) const {
    return ((cluster - 2) * 1) + m_first_data_sector; // Assuming 1 sector per cluster for FAT12 default
}

uint32_t Fat12FileSystem::get_next_cluster(uint32_t cluster) {
    // Validate cluster range to prevent invalid reads
    if (cluster < 2 || cluster >= 4085) {
        return 0x0FFF; // End of chain marker for FAT12
    }
    
    uint32_t fat_offset = cluster + (cluster / 2);
    uint8_t buffer[2];
    auto res = m_device->read(m_fat_sector * 512 + fat_offset, 2, buffer);
    if (res.is_error()) {
        return 0x0FFF; // End of chain marker on error
    }

    uint16_t val = *reinterpret_cast<uint16_t*>(buffer);
    uint32_t next_cluster;
    if (cluster & 1) next_cluster = val >> 4;
    else next_cluster = val & 0x0FFF;
    
    // Validate next cluster and check for infinite loops
    if (next_cluster >= 4085 || next_cluster == cluster) {
        return 0x0FFF; // End of chain marker
    }
    
    return next_cluster;
}

fk::core::Result<size_t, fk::core::Error>
Fat12FileSystem::read(uint64_t, size_t, uint8_t*) {
    return fk::core::Error::IsDirectory;
}

fk::core::Result<size_t, fk::core::Error>
Fat12FileSystem::write(uint64_t, size_t, const uint8_t*) {
    return fk::core::Error::IsDirectory;
}

void Fat12FileSystem::write_fat_entry(uint32_t cluster, uint32_t value) {
    uint32_t fat_offset = cluster + (cluster / 2);
    uint8_t buffer[2];
    auto read_res = m_device->read(m_fat_sector * 512 + fat_offset, 2, buffer);
    if (read_res.is_error()) {
        fk::algorithms::kwarn("FAT12", "write_fat_entry: read failed at cluster %u", cluster);
        return;
    }

    uint16_t val = *reinterpret_cast<uint16_t*>(buffer);
    if (cluster & 1) val = (val & 0x000F) | (value << 4);
    else val = (val & 0xF000) | (value & 0x0FFF);

    auto write_res = m_device->write(m_fat_sector * 512 + fat_offset, 2, reinterpret_cast<uint8_t*>(&val));
    if (write_res.is_error()) {
        fk::algorithms::kwarn("FAT12", "write_fat_entry: write failed at cluster %u", cluster);
    }
}

fk::core::Result<uint32_t, fk::core::Error> Fat12FileSystem::allocate_cluster() {
    // Basic linear search for a free cluster (0)
    for (uint32_t i = 2; i < 4085; ++i) {
        if (get_next_cluster(i) == 0) {
            write_fat_entry(i, 0x0FFF); // Mark as EOF
            return i;
        }
    }
    return fk::core::Error::NoSpaceLeftOnDevice;
}

fk::core::Result<size_t, fk::core::Error>
Fat12FileSystem::read_from_cluster_chain(uint32_t first_cluster, uint64_t offset, size_t size, uint8_t* buffer) {
    constexpr uint32_t CLUSTER_SIZE = 512;
    uint32_t current_cluster = first_cluster;

    uint64_t clusters_to_skip = offset / CLUSTER_SIZE;
    for (uint64_t i = 0; i < clusters_to_skip && current_cluster < 0x0FF8; ++i)
        current_cluster = get_next_cluster(current_cluster);

    uint64_t cluster_offset = offset % CLUSTER_SIZE;
    size_t bytes_read = 0;

    while (bytes_read < size && current_cluster < 0x0FF8) {
        uint8_t temp[512];
        auto res = m_device->read(cluster_to_sector(current_cluster) * 512, 512, temp);
        if (res.is_error()) {
            fk::algorithms::kwarn("FAT12", "read_from_cluster_chain: read failed at cluster %u", current_cluster);
            return bytes_read;
        }
        size_t to_copy = size - bytes_read;
        if (to_copy > (size_t)(CLUSTER_SIZE - cluster_offset))
            to_copy = (size_t)(CLUSTER_SIZE - cluster_offset);
        fk::memory::copy(buffer + bytes_read, temp + cluster_offset, to_copy);
        bytes_read += to_copy;
        cluster_offset = 0;
        current_cluster = get_next_cluster(current_cluster);
    }
    return bytes_read;
}

fk::core::Result<size_t, fk::core::Error>
Fat12FileSystem::write_to_cluster_chain(uint32_t first_cluster, uint64_t offset, size_t size, const uint8_t* buffer) {
    constexpr uint32_t CLUSTER_SIZE = 512;

    if (size == 0) return uint64_t(0);
    if (first_cluster < 2) return fk::core::Error::InvalidParameter;

    uint32_t current_cluster = first_cluster;
    uint64_t bytes_written = 0;

    // Skip clusters to reach the starting offset
    uint64_t clusters_to_skip = offset / CLUSTER_SIZE;
    uint32_t prev_cluster = current_cluster;
    for (uint64_t i = 0; i < clusters_to_skip && current_cluster < 0x0FF8; ++i) {
        prev_cluster = current_cluster;
        current_cluster = get_next_cluster(current_cluster);
    }

    uint64_t cluster_offset = offset % CLUSTER_SIZE;

    while (bytes_written < size) {
        // Allocate clusters if we've reached end-of-chain
        if (current_cluster >= 0x0FF8) {
            auto alloc_result = allocate_cluster();
            if (alloc_result.is_error()) {
                fk::algorithms::kwarn("FAT12", "write_to_cluster_chain: no free clusters");
                if (bytes_written > 0) return bytes_written;
                return alloc_result.error();
            }
            uint32_t new_cluster = alloc_result.value();
            if (current_cluster >= 0x0FF8) {
                // Extending from end-of-chain: link prev -> new
                write_fat_entry(prev_cluster, new_cluster);
            } else {
                // Shouldn't happen if chain is well-formed, but handle gracefully
                write_fat_entry(prev_cluster, new_cluster);
            }
            current_cluster = new_cluster;
        }

        // Write data to this cluster
        uint8_t temp[512];
        uint32_t sector = cluster_to_sector(current_cluster);

        // Read existing data for partial writes
        if (cluster_offset > 0 || (size - bytes_written) < CLUSTER_SIZE) {
            auto read_res = m_device->read(sector * 512, 512, temp);
            if (read_res.is_error()) {
                fk::algorithms::kwarn("FAT12", "write_to_cluster_chain: read-modify-write failed at cluster %u", current_cluster);
                return bytes_written;
            }
        }

        size_t to_write = size - bytes_written;
        if (to_write > CLUSTER_SIZE - cluster_offset)
            to_write = CLUSTER_SIZE - cluster_offset;

        fk::memory::copy(temp + cluster_offset, buffer + bytes_written, to_write);

        auto write_res = m_device->write(sector * 512, 512, temp);
        if (write_res.is_error()) {
            fk::algorithms::kwarn("FAT12", "write_to_cluster_chain: write failed at cluster %u", current_cluster);
            return bytes_written;
        }

        bytes_written += to_write;
        cluster_offset = 0;
        prev_cluster = current_cluster;
        current_cluster = get_next_cluster(current_cluster);
    }

    return bytes_written;
}

static bool fat12_name_eq(const char* raw_name, const char* raw_ext, const char* query) {
    return fk::algorithms::fat_name_eq_83(raw_name, raw_ext, query);
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
Fat12FileSystem::lookup(const char* name) {
    uint8_t sector[512];
    uint32_t root_start = m_first_data_sector - m_root_dir_sectors;
    char lfn_buf[256] = {};
    bool has_lfn = false;

    for (uint32_t i = 0; i < m_root_dir_sectors; ++i) {
        auto res = m_device->read((root_start + i) * 512, 512, sector);
        if (res.is_error()) {
            fk::algorithms::kwarn("FAT12", "lookup: read failed at sector %u", root_start + i);
            return fk::core::Error::IOError;
        }
        auto* dir = reinterpret_cast<Fat12DirectoryEntry*>(sector);
        for (int j = 0; j < 16; ++j) {
            if (dir[j].name[0] == 0) return fk::core::Error::NotFound;
            if (static_cast<uint8_t>(dir[j].name[0]) == 0xE5) { has_lfn = false; continue; }
            if (dir[j].attr == 0x0F) {
                const uint8_t* raw = reinterpret_cast<const uint8_t*>(&dir[j]);
                int order = raw[0] & 0x1F;
                if (raw[0] & 0x40) { fk::memory::set(lfn_buf, 0, sizeof(lfn_buf)); has_lfn = true; }
                if (order >= 1 && order <= 20) fk::algorithms::lfn_fill_chars(raw, order, lfn_buf);
                continue;
            }
            bool match = has_lfn ? fk::algorithms::iequal(lfn_buf, name) : false;
            has_lfn = false;
            if (!match && !fat12_name_eq(dir[j].name, dir[j].ext, name)) continue;
            uint32_t cluster = (static_cast<uint32_t>(dir[j].cluster_high) << 16) | dir[j].cluster_low;
            bool is_dir = (dir[j].attr & 0x10) != 0;
            auto node = fk::adopt_ref(new Fat12Node(fk::RefPtr<Fat12FileSystem>(this), cluster, dir[j].size, is_dir));
            if (!node) return fk::core::Error::OutOfMemory;
            return fk::RefPtr<Node>(node.ptr());
        }
    }
    return fk::core::Error::NotFound;
}

fk::core::Result<void, fk::core::Error>
Fat12FileSystem::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    uint8_t sector[512];
    uint32_t root_start = m_first_data_sector - m_root_dir_sectors;

    for (uint32_t i = 0; i < m_root_dir_sectors; ++i) {
        auto res = m_device->read((root_start + i) * 512, 512, sector);
        if (res.is_error()) {
            fk::algorithms::kwarn("FAT12", "list_dir: read failed at sector %u", root_start + i);
            return fk::core::Error::IOError;
        }
        auto* dir = reinterpret_cast<Fat12DirectoryEntry*>(sector);
        char lfn_buf[256] = {};
        bool has_lfn = false;
        for (int j = 0; j < 16; ++j) {
            if (dir[j].name[0] == 0) return {};
            if (static_cast<uint8_t>(dir[j].name[0]) == 0xE5) { has_lfn = false; continue; }
            if (dir[j].attr == 0x0F) {
                const uint8_t* raw = reinterpret_cast<const uint8_t*>(&dir[j]);
                int order = raw[0] & 0x1F;
                if (raw[0] & 0x40) { fk::memory::set(lfn_buf, 0, sizeof(lfn_buf)); has_lfn = true; }
                if (order >= 1 && order <= 20) fk::algorithms::lfn_fill_chars(raw, order, lfn_buf);
                continue;
            }
            DirectoryEntry entry;
            if (!has_lfn) fk::algorithms::format_83_name(dir[j].name, dir[j].ext, entry.name);
            if (has_lfn) fk::memory::copy(entry.name, lfn_buf, 256);
            has_lfn = false;
            entry.type = (dir[j].attr & 0x10) != 0 ? 1 : 0;
            entries.push_back(entry);
        }
    }
    return {};
}

}
