#include <Kernel/Fs/Fat16/fat_16_fs.h>
#include <Kernel/Fs/Fat16/bpb.h>
#include <Kernel/Fs/Fat16/directory_entry.h>
#include <LibFK/Memory/heap_malloc.h>
#include <LibFK/Utilities/Memory.h>

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
        for (int j = 0; j < 16; ++j) {
            if (dir[j].name[0] == 0) return {};
            if (static_cast<uint8_t>(dir[j].name[0]) == 0xE5) continue;

            DirectoryEntry entry;
            fk::memory::copy(entry.name, dir[j].name, 11);
            entry.name[11] = '\0';
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

}