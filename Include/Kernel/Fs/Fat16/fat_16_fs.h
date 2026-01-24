#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Driver/Storage/storage_device.h>
#include <LibFK/Memory/retain_ptr.h>

namespace fkernel {

class Fat16FileSystem : public Node {
    fk::RefPtr<StorageDevice> m_device;
    uint32_t m_first_data_sector;
    uint32_t m_fat_sector;
    uint32_t m_root_dir_sectors;
    uint32_t m_fat_size;

public:
    static fk::core::Result<fk::RefPtr<Fat16FileSystem>, fk::core::Error>
    create(fk::RefPtr<StorageDevice> device);

    virtual fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t *buffer) override;

    virtual fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t *buffer) override;

    virtual size_t size() const override { return 0; }
    virtual bool is_directory() const override { return true; }

    virtual fk::core::Result<void, fk::core::Error>
    list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;

private:
    Fat16FileSystem(fk::RefPtr<StorageDevice> device) : m_device(device) {}
};

}
