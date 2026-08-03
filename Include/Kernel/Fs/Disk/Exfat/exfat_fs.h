#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Fs/Disk/Exfat/exfat_bpb.h>
#include <Kernel/Driver/Storage/Interfaces/storage_device.h>
#include <LibFK/Container/Sequence/vector.h>

namespace fkernel {

class ExfatNode;

class ExfatFileSystem : public Node {
    friend class ExfatNode;

public:
    fk::RefPtr<StorageDevice> m_device;
    uint32_t m_bytes_per_sector;
    uint32_t m_sectors_per_cluster;
    uint32_t m_bytes_per_cluster;
    uint32_t m_fat_offset;       // byte offset of FAT from volume start
    uint32_t m_heap_offset;      // sector offset of cluster heap from volume start
    uint32_t m_root_cluster;
    uint32_t m_cluster_count;
    uint32_t m_bitmap_cluster;   // first cluster of allocation bitmap
    uint32_t m_bitmap_size;      // size of bitmap in bytes

    static fk::core::Result<fk::RefPtr<ExfatFileSystem>, fk::core::Error>
    create(fk::RefPtr<StorageDevice> device);

    // Root Node interface
    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    size_t size() const override { return 0; }
    bool is_directory() const override { return true; }
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
    fk::core::Result<void, fk::core::Error>
    list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    create_child(const char* name, int mode) override;
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    mkdir(const char* name, int mode) override;
    fk::core::Result<void, fk::core::Error> unlink(const char* name) override;
    fk::core::Result<void, fk::core::Error> rmdir(const char* name) override;

    // Cluster I/O helpers
    uint64_t cluster_to_byte(uint32_t cluster) const;
    fk::core::Result<void, fk::core::Error> read_cluster(uint32_t cluster, uint8_t* buf);
    fk::core::Result<void, fk::core::Error> write_cluster(uint32_t cluster, const uint8_t* buf);

    // FAT helpers
    uint32_t next_fat(uint32_t cluster);
    fk::core::Result<void, fk::core::Error> set_fat(uint32_t cluster, uint32_t value);
    fk::core::Result<uint32_t, fk::core::Error> alloc_cluster(uint32_t prev);
    void free_chain(uint32_t first_cluster);

    // Bitmap helpers
    bool bitmap_get(uint32_t cluster);
    fk::core::Result<void, fk::core::Error> bitmap_set(uint32_t cluster, bool used);

    // Cluster chain I/O
    fk::core::Result<size_t, fk::core::Error>
    read_chain(uint32_t first_cluster, bool no_fat_chain, uint64_t data_length,
               uint64_t offset, size_t size, uint8_t* buf);
    fk::core::Result<size_t, fk::core::Error>
    write_chain(uint32_t& first_cluster, bool no_fat_chain, uint64_t& data_length,
                uint64_t offset, size_t size, const uint8_t* buf);

    // Directory operations
    fk::core::Result<void, fk::core::Error>
    list_dir_cluster(uint32_t first_cluster, fk::containers::Vector<DirectoryEntry>& entries);
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup_cluster(uint32_t first_cluster, const char* name);
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    create_entry(uint32_t dir_cluster, const char* name, bool is_dir);
    fk::core::Result<void, fk::core::Error>
    delete_entry(uint32_t dir_cluster, const char* name);
    fk::core::Result<void, fk::core::Error>
    update_stream_ext(uint32_t dir_cluster, uint32_t dir_offset, uint64_t new_size);

private:
    ExfatFileSystem(fk::RefPtr<StorageDevice> device) : m_device(device) {}
};

} // namespace fkernel
