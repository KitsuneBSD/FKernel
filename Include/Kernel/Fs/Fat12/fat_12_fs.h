#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Driver/Storage/storage_device.h>
#include <LibFK/Memory/retain_ptr.h>

namespace fkernel {

class Fat12FileSystem : public Node {
    fk::RefPtr<StorageDevice> m_device;
    uint32_t m_first_data_sector;
    uint32_t m_fat_sector;
    uint32_t m_root_dir_sectors;

public:
    static fk::core::Result<fk::RefPtr<Fat12FileSystem>, fk::core::Error>
    create(fk::RefPtr<StorageDevice> device);

    virtual fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t *buffer) override;

    virtual fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t *buffer) override;

    virtual size_t size() const override { return 0; }
    virtual bool is_directory() const override { return true; }

    virtual fk::core::Result<void, fk::core::Error>
    list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;

    virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup(const char* name) override;

    fk::core::Result<size_t, fk::core::Error>
    read_from_cluster_chain(uint32_t first_cluster, uint64_t offset, size_t size, uint8_t* buffer);

    fk::core::Result<size_t, fk::core::Error>
    write_to_cluster_chain(uint32_t first_cluster, uint64_t offset, size_t size, const uint8_t* buffer);

    uint32_t cluster_to_sector(uint32_t cluster) const;
    uint32_t get_next_cluster(uint32_t cluster);

private:
    Fat12FileSystem(fk::RefPtr<StorageDevice> device) : m_device(device) {}

    void write_fat_entry(uint32_t cluster, uint32_t value);
    fk::core::Result<uint32_t, fk::core::Error> allocate_cluster();
};

}
