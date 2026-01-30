#include <Kernel/Fs/Fat32/fat_32_fs.h>
#include <Kernel/Fs/Fat32/bpb.h>
#include <Kernel/Fs/Fat32/directory_entry.h>
#include <LibFK/Memory/heap_malloc.h>
#include <LibFK/Utilities/Memory.h>
#include <LibFK/Utilities/size_checking.h>

#include <Kernel/Fs/Fat32/fat_32_node.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> 
Fat32FileSystem::lookup(const char* name) {
    fk::containers::Vector<DirectoryEntry> entries;
    list_dir(entries);
    
    // Search root directory
    for (auto& entry : entries) {
        if (strcmp(entry.name, name) == 0) {
            // We need to find the cluster and size for this entry.
            // This requires re-reading the directory entries to get metadata.
            // Simplified: for now return NotFound or implement a proper entry finder.
        }
    }
    return fk::core::Error::NotFound;
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
    uint32_t current_cluster = m_root_cluster;
    uint8_t sector[512];
    uint32_t max_clusters = 10000; // Prevent infinite loops
    uint32_t cluster_count = 0;

    while (current_cluster < 0x0FFFFFF8 && cluster_count < max_clusters) {
        uint32_t root_sector = cluster_to_sector(current_cluster);
        for (uint32_t i = 0; i < m_sectors_per_cluster; ++i) {
            auto res = m_device->read((root_sector + i) * 512, 512, sector);
            if (res.is_error()) {
                return fk::core::Error::IOError;
            }
            auto* dir = reinterpret_cast<Fat32DirectoryEntry*>(sector);
            for (int j = 0; j < 16; ++j) {
                if (dir[j].name[0] == 0) return {};
                if (static_cast<uint8_t>(dir[j].name[0]) == 0xE5) continue;
                
                // Skip Long File Name entries for now (attribute 0x0F)
                if (dir[j].attr == 0x0F) continue;

                DirectoryEntry entry;
                
                // Format FAT 8.3 name
                int name_len = 8;
                while (name_len > 0 && dir[j].name[name_len - 1] == ' ') name_len--;
                
                int ext_len = 3;
                while (ext_len > 0 && dir[j].ext[ext_len - 1] == ' ') ext_len--;
                
                int pos = 0;
                for (int k = 0; k < name_len; ++k) entry.name[pos++] = dir[j].name[k];
                if (ext_len > 0) {
                    entry.name[pos++] = '.';
                    for (int k = 0; k < ext_len; ++k) entry.name[pos++] = dir[j].ext[k];
                }
                entry.name[pos] = '\0';

                entry.type = (dir[j].attr & 0x10) != 0 ? 1 : 0;
                entries.push_back(entry);
            }
        }
        current_cluster = get_next_cluster(current_cluster);
        cluster_count++;
    }
    return {};
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
        
        size_t to_copy = fk::utilities::min(size - bytes_read, (size_t)(cluster_size - cluster_offset));
        fk::memory::copy(buffer + bytes_read, temp + cluster_offset, to_copy);
        
        kfree(temp);
        bytes_read += to_copy;
        cluster_offset = 0;
        current_cluster = get_next_cluster(current_cluster);
    }
    return bytes_read;
}

}
