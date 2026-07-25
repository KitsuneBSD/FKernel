#include <Kernel/Fs/Fat16/fat_16_fs.h>
#include <Kernel/Fs/Fat16/fat_16_node.h>
#include <Kernel/Fs/Fat16/bpb.h>
#include <Kernel/Fs/Fat16/directory_entry.h>
#include <LibFK/Algorithms/fat_name.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Algorithms/string_algorithms.h>
#include <LibFK/Memory/heap_malloc.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<Fat16FileSystem>, fk::core::Error>
Fat16FileSystem::create(fk::RefPtr<StorageDevice> device) {
    Fat16Bpb bpb;
    if (device->read(0, sizeof(bpb), reinterpret_cast<uint8_t*>(&bpb)).is_error())
        return fk::core::Error::IOError;

    // Basic validation for FAT16
    if (bpb.bytes_per_sector != 512) return fk::core::Error::InvalidData;
    if (bpb.sectors_per_cluster == 0) return fk::core::Error::InvalidData;
    if (bpb.root_entry_count == 0) return fk::core::Error::InvalidData;
    if (bpb.fat_count == 0 || bpb.fat_count > 2) return fk::core::Error::InvalidData;
    if (bpb.fat_size_16 == 0) return fk::core::Error::InvalidData;
    
    // Check filesystem type signature
    if (fk::memory::compare(bpb.fs_type, "FAT16   ", 8) != 0 && 
        fk::memory::compare(bpb.fs_type, "FAT12   ", 8) != 0) {
        return fk::core::Error::InvalidData;
    }
    
    uint32_t total_sectors = bpb.total_sectors_16 != 0 ? bpb.total_sectors_16 : bpb.total_sectors_32;
    if (total_sectors == 0) return fk::core::Error::InvalidData;
    
    uint32_t root_dir_sectors = (bpb.root_entry_count * 32) / 512;
    uint32_t data_sectors = total_sectors - (bpb.reserved_sectors + (bpb.fat_count * bpb.fat_size_16) + root_dir_sectors);
    if (data_sectors <= 0) return fk::core::Error::InvalidData;
    
    uint32_t clusters = data_sectors / bpb.sectors_per_cluster;
    if (clusters < 4085 || clusters >= 65525) return fk::core::Error::InvalidData;

    auto fs = fk::adopt_ref(new Fat16FileSystem(device));
    if (!fs) return fk::core::Error::OutOfMemory;

    fs->m_fat_sector = bpb.reserved_sectors;
    fs->m_root_dir_sectors = root_dir_sectors;
    fs->m_first_data_sector = bpb.reserved_sectors + (bpb.fat_count * bpb.fat_size_16) + root_dir_sectors;
    fs->m_fat_size = bpb.fat_size_16;
    fs->m_sectors_per_cluster = bpb.sectors_per_cluster;

    return fs;
}

fk::core::Result<void, fk::core::Error>
Fat16FileSystem::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    uint8_t sector[512];
    uint32_t root_start = m_first_data_sector - m_root_dir_sectors;

    for (uint32_t i = 0; i < m_root_dir_sectors; ++i) {
        auto res = m_device->read((root_start + i) * 512, 512, sector);
        if (res.is_error()) {
            return fk::core::Error::IOError;
        }
        
        auto* dir = reinterpret_cast<Fat16DirectoryEntry*>(sector);
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

fk::core::Result<size_t, fk::core::Error>
Fat16FileSystem::read(uint64_t, size_t, uint8_t*) {
    return fk::core::Error::IsDirectory;
}

fk::core::Result<size_t, fk::core::Error>
Fat16FileSystem::write(uint64_t, size_t, const uint8_t*) {
    return fk::core::Error::IsDirectory;
}

uint32_t Fat16FileSystem::cluster_to_sector(uint32_t cluster) const {
    return ((cluster - 2) * m_sectors_per_cluster) + m_first_data_sector;
}

uint32_t Fat16FileSystem::get_next_cluster(uint32_t cluster) {
    if (cluster < 2 || cluster >= 0xFFF8) return 0xFFFF;
    uint16_t val = 0;
    auto res = m_device->read(m_fat_sector * 512 + cluster * 2, 2, reinterpret_cast<uint8_t*>(&val));
    if (res.is_error()) {
        fk::algorithms::kwarn("FAT16", "get_next_cluster: read failed at cluster %u", cluster);
        return 0xFFFF;
    }
    return val;
}

fk::core::Result<size_t, fk::core::Error>
Fat16FileSystem::read_from_cluster_chain(uint32_t first_cluster, uint64_t offset, size_t size, uint8_t* buffer) {
    uint32_t cluster_size = m_sectors_per_cluster * 512;
    uint32_t current_cluster = first_cluster;

    uint64_t clusters_to_skip = offset / cluster_size;
    for (uint64_t i = 0; i < clusters_to_skip && current_cluster < 0xFFF8; ++i)
        current_cluster = get_next_cluster(current_cluster);

    uint64_t cluster_offset = offset % cluster_size;
    size_t bytes_read = 0;

    while (bytes_read < size && current_cluster < 0xFFF8) {
        uint8_t* temp = static_cast<uint8_t*>(kmalloc(cluster_size));
        if (!temp) {
            fk::algorithms::kwarn("FAT16", "read_from_cluster_chain: alloc failed");
            return bytes_read;
        }
        auto read_res = m_device->read(cluster_to_sector(current_cluster) * 512, cluster_size, temp);
        if (read_res.is_error()) {
            fk::algorithms::kwarn("FAT16", "read_from_cluster_chain: read failed at cluster %u", current_cluster);
            kfree(temp);
            return bytes_read;
        }
        size_t to_copy = size - bytes_read;
        if (to_copy > (size_t)(cluster_size - cluster_offset))
            to_copy = (size_t)(cluster_size - cluster_offset);
        fk::memory::copy(buffer + bytes_read, temp + cluster_offset, to_copy);
        kfree(temp);
        bytes_read += to_copy;
        cluster_offset = 0;
        current_cluster = get_next_cluster(current_cluster);
    }
    return bytes_read;
}


fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
Fat16FileSystem::lookup(const char* name) {
    uint8_t sector[512];
    uint32_t root_start = m_first_data_sector - m_root_dir_sectors;
    char lfn_buf[256] = {};
    bool has_lfn = false;

    for (uint32_t i = 0; i < m_root_dir_sectors; ++i) {
        auto res = m_device->read((root_start + i) * 512, 512, sector);
        if (res.is_error()) {
            fk::algorithms::kwarn("FAT16", "lookup: read failed at sector %u", root_start + i);
            return fk::core::Error::IOError;
        }
        auto* dir = reinterpret_cast<Fat16DirectoryEntry*>(sector);
        for (int j = 0; j < 16; ++j) {
            if (dir[j].name[0] == 0) return fk::core::Error::NotFound;
            if (static_cast<uint8_t>(dir[j].name[0]) == 0xE5) { has_lfn = false; continue; }
            if (dir[j].attr == 0x0F) {
                const uint8_t* raw = reinterpret_cast<const uint8_t*>(&dir[j]);
                int order = raw[0] & 0x1F;
                if (raw[0] & 0x40) { fk::memory::set(lfn_buf, 0, 256); has_lfn = true; }
                if (order >= 1 && order <= 20) fk::algorithms::lfn_fill_chars(raw, order, lfn_buf);
                continue;
            }
            bool match = has_lfn ? fk::algorithms::iequal(lfn_buf, name) : false;
            has_lfn = false;
            if (!match) {
                char sfn[13]; fk::algorithms::format_83_name(dir[j].name, dir[j].ext, sfn);
                match = fk::algorithms::iequal(sfn, name);
            }
            if (!match) continue;
            uint32_t cluster = (static_cast<uint32_t>(dir[j].cluster_high) << 16) | dir[j].cluster_low;
            bool is_dir = (dir[j].attr & 0x10) != 0;
            auto node = fk::adopt_ref(new Fat16Node(fk::RefPtr<Fat16FileSystem>(this), cluster, dir[j].size, is_dir));
            if (!node) return fk::core::Error::OutOfMemory;
            return fk::RefPtr<Node>(node.ptr());
        }
    }
    return fk::core::Error::NotFound;
}

}