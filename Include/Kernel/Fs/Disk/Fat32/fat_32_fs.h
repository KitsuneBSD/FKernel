#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Fs/Disk/Fat32/directory_entry.h>
#include <Kernel/Driver/Storage/storage_device.h>

namespace fkernel {

class Fat32Node;

class Fat32FileSystem : public Node {
    friend class Fat32Node;
    fk::RefPtr<StorageDevice> m_device;
    uint32_t m_first_data_sector;
    uint32_t m_fat_sector;
    uint32_t m_root_cluster;
    uint32_t m_sectors_per_cluster;
    uint32_t m_fat_size_sectors;
    uint32_t m_total_clusters;

public:
    static fk::core::Result<fk::RefPtr<Fat32FileSystem>, fk::core::Error>
    create(fk::RefPtr<StorageDevice> device);

    virtual fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t *buffer) override;

    virtual fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t *buffer) override;

    virtual size_t size() const override { return 0; }
    virtual bool is_directory() const override { return true; }

    virtual fk::core::Result<void, fk::core::Error>
    list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;

    virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;

    fk::core::Result<size_t, fk::core::Error>
    read_from_cluster_chain(uint32_t first_cluster, uint64_t offset, size_t size, uint8_t* buffer);

    fk::core::Result<size_t, fk::core::Error>
    write_to_cluster_chain(uint32_t& first_cluster, uint64_t offset, size_t size,
                           const uint8_t* buffer, size_t& file_size_inout);

    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    find_in_directory(uint32_t first_cluster, const char* name);

    fk::core::Result<void, fk::core::Error>
    list_directory_from(uint32_t first_cluster, fk::containers::Vector<DirectoryEntry>& entries);

    fk::core::Result<void, fk::core::Error>
    update_dir_entry_size(uint32_t dir_sector_lba, uint8_t entry_idx, uint32_t new_size);

    // Metadata operations for writable FAT32
    fk::core::Result<void, fk::core::Error>
    find_free_dir_entry(uint32_t dir_cluster, uint32_t& out_sector, uint8_t& out_idx,
                        bool allow_cluster_expand = true);

    fk::core::Result<void, fk::core::Error>
    write_dir_entry_raw(uint32_t sector_lba, uint8_t entry_idx, const Fat32DirectoryEntry& entry);

    void free_cluster_chain(uint32_t first_cluster);

    fk::core::Result<uint32_t, fk::core::Error>
    allocate_cluster(uint32_t prev_cluster);

private:
    Fat32FileSystem(fk::RefPtr<StorageDevice> device) : m_device(device) {}
    uint32_t cluster_to_sector(uint32_t cluster) const;
    uint32_t get_next_cluster(uint32_t cluster);
    fk::core::Result<void, fk::core::Error> write_fat_entry(uint32_t cluster, uint32_t value);
};

}
