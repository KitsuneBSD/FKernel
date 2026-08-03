#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Fs/Disk/Iso9660/iso9660_vd.h>
#include <Kernel/Driver/Storage/Interfaces/storage_device.h>
#include <LibFK/Text/string.h>

namespace fkernel {

class Iso9660Node;

class Iso9660FileSystem : public Node {
    friend class Iso9660Node;

public:
    fk::RefPtr<StorageDevice> m_device;
    uint32_t m_root_lba;
    uint32_t m_root_size;
    bool     m_joliet;
    uint32_t m_joliet_root_lba;
    uint32_t m_joliet_root_size;
    bool     m_rock_ridge;

    static fk::core::Result<fk::RefPtr<Iso9660FileSystem>, fk::core::Error>
    create(fk::RefPtr<StorageDevice> device);

    // Read-only: root directory Node interface
    fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buffer) override;
    fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    size_t size() const override { return 0; }
    bool is_directory() const override { return true; }
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup(const char* name) override;
    fk::core::Result<void, fk::core::Error>
    list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;

    // Write ops always fail (read-only)
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    create_child(const char* name, int mode) override { (void)name;(void)mode; return fk::core::Error::ReadOnly; }
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    mkdir(const char* name, int mode) override { (void)name;(void)mode; return fk::core::Error::ReadOnly; }
    fk::core::Result<void, fk::core::Error>
    unlink(const char* name) override { (void)name; return fk::core::Error::ReadOnly; }
    fk::core::Result<void, fk::core::Error>
    rmdir(const char* name) override { (void)name; return fk::core::Error::ReadOnly; }

    // Helpers used by Iso9660Node
    fk::core::Result<void, fk::core::Error>
    read_sector(uint32_t lba, uint8_t* buf);
    fk::core::Result<size_t, fk::core::Error>
    read_bytes(uint32_t lba, uint64_t offset, size_t size, uint8_t* buf);
    fk::core::Result<void, fk::core::Error>
    list_dir_lba(uint32_t lba, uint32_t data_size,
                 fk::containers::Vector<DirectoryEntry>& entries);
    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    lookup_lba(uint32_t lba, uint32_t data_size, const char* name);

    // Parse one 2048-byte VD sector buffer; sets joliet fields if SVD Joliet found
    bool parse_vd(const uint8_t* vd, bool& got_pvd);

    // Strip `;N` version suffix and copy to out (null-terminated)
    static void strip_version(const char* src, uint8_t len, char* out);

    // Convert Joliet UCS-2 BE name to ASCII
    static void joliet_to_ascii(const uint8_t* ucs2, uint8_t len_bytes, char* out);

    // Extract Rock Ridge NM alternate name from system use area
    bool rr_get_nm(const uint8_t* su_start, uint8_t su_len, char* out, size_t out_max);

    // Extract Rock Ridge SL symlink target from system use area
    bool rr_get_sl(const uint8_t* su_start, uint8_t su_len, char* out, size_t out_max);

    // Build an Iso9660Node from a raw directory record buffer pointer
    fk::RefPtr<Node> make_node_from_dr(const uint8_t* dr);

private:
    Iso9660FileSystem(fk::RefPtr<StorageDevice> device) : m_device(device) {}
};

} // namespace fkernel
