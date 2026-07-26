#include <Kernel/Fs/Disk/Fat32/fat_32_fs.h>
#include <Kernel/Fs/Disk/Fat32/bpb.h>
#include <Kernel/Fs/Disk/Fat32/directory_entry.h>
#include <LibFK/Algorithms/fat_name.h>
#include <LibFK/Algorithms/string_algorithms.h>
#include <LibFK/Memory/heap_malloc.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/math.h>

#include <Kernel/Fs/Disk/Fat32/fat_32_node.h>

namespace fkernel {

fk::core::Result<void, fk::core::Error>
Fat32FileSystem::list_directory_from(uint32_t first_cluster, fk::containers::Vector<DirectoryEntry>& entries) {
    uint32_t current_cluster = first_cluster;
    uint8_t sector[512];
    uint32_t cluster_count = 0;
    char lfn_buf[256] = {};
    bool has_lfn = false;

    while (current_cluster < 0x0FFFFFF8 && cluster_count < 10000) {
        uint32_t root_sector = cluster_to_sector(current_cluster);
        for (uint32_t i = 0; i < m_sectors_per_cluster; ++i) {
            if (m_device->read((root_sector + i) * 512, 512, sector).is_error())
                return fk::core::Error::IOError;
            auto* dir = reinterpret_cast<Fat32DirectoryEntry*>(sector);
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
        current_cluster = get_next_cluster(current_cluster);
        cluster_count++;
    }
    return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
Fat32FileSystem::find_in_directory(uint32_t first_cluster, const char* name) {
    uint32_t current_cluster = first_cluster;
    uint8_t sector[512];
    uint32_t cluster_count = 0;
    char lfn_buf[256] = {};
    bool has_lfn = false;

    while (current_cluster < 0x0FFFFFF8 && cluster_count < 10000) {
        uint32_t root_sector = cluster_to_sector(current_cluster);
        for (uint32_t i = 0; i < m_sectors_per_cluster; ++i) {
            if (m_device->read((root_sector + i) * 512, 512, sector).is_error())
                return fk::core::Error::IOError;
            auto* dir = reinterpret_cast<Fat32DirectoryEntry*>(sector);
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
                if (!match) {
                    char sfn[13];
                    fk::algorithms::format_83_name(dir[j].name, dir[j].ext, sfn);
                    match = fk::algorithms::iequal(sfn, name);
                }
                if (!match) continue;
                uint32_t cluster = (static_cast<uint32_t>(dir[j].cluster_high) << 16) | dir[j].cluster_low;
                bool is_dir = (dir[j].attr & 0x10) != 0;
                uint32_t dir_sector_lba = root_sector + i;
                auto node = fk::adopt_ref(new Fat32Node(fk::RefPtr<Fat32FileSystem>(this), cluster,
                                                        dir[j].size, is_dir,
                                                        dir_sector_lba, (uint8_t)j));
                if (!node) return fk::core::Error::OutOfMemory;
                return fk::RefPtr<Node>(node.ptr());
            }
        }
        current_cluster = get_next_cluster(current_cluster);
        cluster_count++;
    }
    return fk::core::Error::NotFound;
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
Fat32FileSystem::lookup(const char* name) {
    return find_in_directory(m_root_cluster, name);
}

fk::core::Result<fk::RefPtr<Fat32FileSystem>, fk::core::Error>
Fat32FileSystem::create(fk::RefPtr<StorageDevice> device) {
    Fat32Bpb bpb;
    if (device->read(0, sizeof(bpb), reinterpret_cast<uint8_t*>(&bpb)).is_error())
        return fk::core::Error::IOError;

    // Basic validation for FAT32
    if (bpb.bytes_per_sector != 512) {
        fk::algorithms::kwarn("FAT32", "Invalid bytes per sector: %u", bpb.bytes_per_sector);
        return fk::core::Error::InvalidData;
    }
    if (bpb.sectors_per_cluster == 0) {
        fk::algorithms::kwarn("FAT32", "Invalid sectors per cluster: 0");
        return fk::core::Error::InvalidData;
    }
    if (bpb.root_entry_count != 0) {
        fk::algorithms::kwarn("FAT32", "Invalid root entry count: %u (expected 0 for FAT32)", bpb.root_entry_count);
        // return fk::core::Error::InvalidData; // Some formatters might be loose here
    }
    if (bpb.fat_size_16 != 0) {
        fk::algorithms::kwarn("FAT32", "Invalid FAT size 16: %u (expected 0 for FAT32)", bpb.fat_size_16);
        // return fk::core::Error::InvalidData;
    }
    if (bpb.fat_count == 0 || bpb.fat_count > 2) {
        fk::algorithms::kwarn("FAT32", "Invalid FAT count: %u", bpb.fat_count);
        return fk::core::Error::InvalidData;
    }
    if (bpb.fat_size_32 == 0) {
        fk::algorithms::kwarn("FAT32", "Invalid FAT size 32: 0");
        return fk::core::Error::InvalidData;
    }
    
    // Check filesystem type signature
    if (fk::memory::compare(bpb.fs_type, "FAT32   ", 8) != 0) {
        char type_str[9];
        fk::memory::copy(type_str, bpb.fs_type, 8);
        type_str[8] = '\0';
        fk::algorithms::kwarn("FAT32", "Invalid fs_type signature: '%s'", type_str);
        // return fk::core::Error::InvalidData; // Many modern tools put garbage or different strings here
    }
    
    // Validate cluster count - should be reasonable
    uint32_t total_sectors = bpb.total_sectors_32 ? bpb.total_sectors_32 : bpb.total_sectors_16;
    if (total_sectors == 0) {
        fk::algorithms::kwarn("FAT32", "Total sectors is 0");
        return fk::core::Error::InvalidData;
    }
    
    uint32_t data_sectors = total_sectors - bpb.reserved_sectors - (bpb.fat_count * bpb.fat_size_32);
    uint32_t total_clusters = data_sectors / bpb.sectors_per_cluster;
    if (total_clusters < 65525) {
        fk::algorithms::kwarn("FAT32", "Too few clusters for FAT32: %u (expected > 65525)", total_clusters);
        // return fk::core::Error::InvalidData;
    }
    
    // Validate root cluster - should be 2 or higher
    if (bpb.root_cluster < 2) {
        fk::algorithms::kwarn("FAT32", "Invalid root cluster: %u", bpb.root_cluster);
        return fk::core::Error::InvalidData;
    }

    fk::algorithms::klog("FAT32", "Validation successful. Root cluster: %u, Sectors/Cluster: %u", bpb.root_cluster, bpb.sectors_per_cluster);

    auto fs = fk::adopt_ref(new Fat32FileSystem(device));
    if (!fs) return fk::core::Error::OutOfMemory;

    fs->m_fat_sector = bpb.reserved_sectors;
    fs->m_root_cluster = bpb.root_cluster;
    fs->m_sectors_per_cluster = bpb.sectors_per_cluster;
    fs->m_first_data_sector = bpb.reserved_sectors + (bpb.fat_count * bpb.fat_size_32);
    fs->m_fat_size_sectors = bpb.fat_size_32;
    fs->m_total_clusters = total_clusters;

    return fs;
}

uint32_t Fat32FileSystem::cluster_to_sector(uint32_t cluster) const {
    return ((cluster - 2) * m_sectors_per_cluster) + m_first_data_sector;
}

uint32_t Fat32FileSystem::get_next_cluster(uint32_t cluster) {
    // Validate cluster range to prevent invalid reads
    if (cluster < 2 || cluster > 0x0FFFFFFF) {
        return 0x0FFFFFFF; // End of chain marker
    }
    
    uint32_t fat_offset = cluster * 4;
    uint32_t val = 0;
    auto res = m_device->read(m_fat_sector * 512 + fat_offset, 4, reinterpret_cast<uint8_t*>(&val));
    if (res.is_error()) {
        return 0x0FFFFFFF; // End of chain marker on error
    }
    
    uint32_t next_cluster = val & 0x0FFFFFFF;
    
    // Validate next cluster to prevent infinite loops
    if (next_cluster < 2 || next_cluster > 0x0FFFFFFF || next_cluster == cluster) {
        return 0x0FFFFFFF; // End of chain marker
    }
    
    return next_cluster;
}

fk::core::Result<size_t, fk::core::Error>
Fat32FileSystem::read(uint64_t, size_t, uint8_t*) {
    return fk::core::Error::IsDirectory;
}

fk::core::Result<size_t, fk::core::Error>
Fat32FileSystem::write(uint64_t, size_t, const uint8_t*) {
    return fk::core::Error::IsDirectory;
}

fk::core::Result<void, fk::core::Error>
Fat32FileSystem::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    return list_directory_from(m_root_cluster, entries);
}

fk::core::Result<size_t, fk::core::Error>
Fat32FileSystem::read_from_cluster_chain(uint32_t first_cluster, uint64_t offset, size_t size, uint8_t* buffer) {
    uint32_t current_cluster = first_cluster;
    uint32_t cluster_size = m_sectors_per_cluster * 512;

    uint64_t clusters_to_skip = offset / cluster_size;
    for (uint64_t i = 0; i < clusters_to_skip; ++i) {
        current_cluster = get_next_cluster(current_cluster);
        if (current_cluster >= 0x0FFFFFF8) return 0;
    }

    uint64_t cluster_offset = offset % cluster_size;
    size_t bytes_read = 0;

    while (bytes_read < size && current_cluster < 0x0FFFFFF8) {
        uint8_t* temp = static_cast<uint8_t*>(kmalloc(cluster_size));
        m_device->read(cluster_to_sector(current_cluster) * 512, cluster_size, temp);
        
        size_t to_copy = fk::algorithms::min(size - bytes_read, (size_t)(cluster_size - cluster_offset));
        fk::memory::copy(buffer + bytes_read, temp + cluster_offset, to_copy);
        
        kfree(temp);
        bytes_read += to_copy;
        cluster_offset = 0;
        current_cluster = get_next_cluster(current_cluster);
    }
    return bytes_read;
}

fk::core::Result<void, fk::core::Error>
Fat32FileSystem::write_fat_entry(uint32_t cluster, uint32_t value) {
    uint32_t fat_byte_offset = cluster * 4;
    uint32_t sector_offset = fat_byte_offset / 512;
    uint32_t byte_in_sector = fat_byte_offset % 512;

    uint8_t sector_buf[512];
    auto res = m_device->read((m_fat_sector + sector_offset) * 512, 512, sector_buf);
    if (res.is_error()) return fk::core::Error::IOError;

    // Preserve the high 4 bits (reserved by FAT32 spec)
    uint32_t old_val;
    fk::memory::copy(&old_val, sector_buf + byte_in_sector, 4);
    uint32_t new_val = (old_val & 0xF0000000u) | (value & 0x0FFFFFFFu);
    fk::memory::copy(sector_buf + byte_in_sector, &new_val, 4);

    auto wres = m_device->write((m_fat_sector + sector_offset) * 512, 512, sector_buf);
    if (wres.is_error()) return wres.error();
    return {};
}

fk::core::Result<uint32_t, fk::core::Error>
Fat32FileSystem::allocate_cluster(uint32_t prev_cluster) {
    uint8_t sector_buf[512];
    uint32_t current_sector = ~0u;

    for (uint32_t cluster = 2; cluster < m_total_clusters + 2; ++cluster) {
        uint32_t fat_byte = cluster * 4;
        uint32_t sector_idx = fat_byte / 512;
        uint32_t byte_in_sector = fat_byte % 512;

        if (sector_idx != current_sector) {
            auto res = m_device->read((m_fat_sector + sector_idx) * 512, 512, sector_buf);
            if (res.is_error()) return fk::core::Error::IOError;
            current_sector = sector_idx;
        }

        uint32_t val;
        fk::memory::copy(&val, sector_buf + byte_in_sector, 4);
        if ((val & 0x0FFFFFFF) != 0) continue;

        // Mark new cluster as end-of-chain
        auto res = write_fat_entry(cluster, 0x0FFFFFFF);
        if (res.is_error()) return res.error();

        // Link previous cluster to new one
        if (prev_cluster >= 2 && prev_cluster < 0x0FFFFFF8) {
            res = write_fat_entry(prev_cluster, cluster);
            if (res.is_error()) return res.error();
        }
        return cluster;
    }
    return fk::core::Error::OutOfMemory;
}

fk::core::Result<size_t, fk::core::Error>
Fat32FileSystem::write_to_cluster_chain(uint32_t& first_cluster, uint64_t offset,
                                         size_t size, const uint8_t* buf,
                                         size_t& file_size_inout) {
    if (size == 0) return (size_t)0;

    uint32_t cluster_size = m_sectors_per_cluster * 512;

    // Allocate first cluster if file is empty
    if (first_cluster < 2) {
        auto res = allocate_cluster(0);
        if (res.is_error()) return res.error();
        first_cluster = res.value();
    }

    // Walk to the cluster containing `offset`, allocating as needed
    uint64_t clusters_to_skip = offset / cluster_size;
    uint32_t current_cluster = first_cluster;
    uint32_t prev_cluster = 0;

    for (uint64_t i = 0; i < clusters_to_skip; ++i) {
        uint32_t next = get_next_cluster(current_cluster);
        if (next >= 0x0FFFFFF8) {
            auto res = allocate_cluster(current_cluster);
            if (res.is_error()) return res.error();
            next = res.value();
        }
        prev_cluster = current_cluster;
        current_cluster = next;
    }
    (void)prev_cluster;

    uint64_t cluster_offset = offset % cluster_size;
    size_t bytes_written = 0;
    uint8_t* cluster_buf = static_cast<uint8_t*>(kmalloc(cluster_size));
    if (!cluster_buf) return fk::core::Error::OutOfMemory;

    while (bytes_written < size) {
        uint32_t sector = cluster_to_sector(current_cluster);
        m_device->read(sector * 512, cluster_size, cluster_buf);

        size_t to_write = fk::algorithms::min(size - bytes_written,
                                              (size_t)(cluster_size - cluster_offset));
        fk::memory::copy(cluster_buf + cluster_offset, buf + bytes_written, to_write);
        m_device->write(sector * 512, cluster_size, cluster_buf);

        bytes_written += to_write;
        cluster_offset = 0;

        if (bytes_written < size) {
            uint32_t next = get_next_cluster(current_cluster);
            if (next >= 0x0FFFFFF8) {
                auto res = allocate_cluster(current_cluster);
                if (res.is_error()) { kfree(cluster_buf); return res.error(); }
                next = res.value();
            }
            current_cluster = next;
        }
    }

    kfree(cluster_buf);

    uint64_t end = offset + bytes_written;
    if (end > file_size_inout) file_size_inout = (size_t)end;
    return bytes_written;
}

fk::core::Result<void, fk::core::Error>
Fat32FileSystem::update_dir_entry_size(uint32_t dir_sector_lba, uint8_t entry_idx, uint32_t new_size) {
    uint8_t sector[512];
    if (m_device->read((uint64_t)dir_sector_lba * 512, 512, sector).is_error())
        return fk::core::Error::IOError;
    auto* dir = reinterpret_cast<Fat32DirectoryEntry*>(sector);
    dir[entry_idx].size = new_size;
    if (m_device->write((uint64_t)dir_sector_lba * 512, 512, sector).is_error())
        return fk::core::Error::IOError;
    return {};
}

}
