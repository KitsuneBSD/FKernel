#pragma once

#include <Kernel/Block/BlockDevice.h>
#include <Kernel/Block/PartitionDevice.h> // For fkernel::block::PartitionBlockDevice
#include <Kernel/FileSystem/VirtualFS/filesystem.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h> // For VNodeOps
#include <LibFK/Memory/retain_ptr.h>
#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Memory/optional.h>
#include <LibFK/Types/types.h> // For uint32_t

namespace fkernel::drivers::ata {

/**
 * @brief A basic "raw block access" filesystem for ATA partitions.
 *
 * This class implements a minimal Filesystem interface, primarily to allow
 * a raw ATA partition to be mounted through the VFS, exposing its contents
 * as a single block device file. It delegates read/write operations directly
 * to the underlying PartitionBlockDevice.
 *
 * This is primarily for demonstrating integration of a new filesystem driver
 * within the ATA driver directory as per user request.
 */
class AtaFs : public ::fkernel::fs::Filesystem {
public:
    /**
     * @brief Constructs an AtaFs instance.
     *
     * @param block_device The underlying partition block device.
     * @param first_sector The starting sector of this AtaFs on the device.
     */
    explicit AtaFs(fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice> block_device,
                   uint32_t first_sector);

    /**
     * @brief Initializes the AtaFs.
     * Creates a dummy root VNode representing the raw disk.
     * @return 0 on success.
     */
    int initialize() override;

    /**
     * @brief Returns the root VNode of this filesystem.
     * @return RetainPtr to the root VNode.
     */
    fk::memory::RetainPtr<VNode> root_vnode() override;

    /**
     * @brief Returns the type of this filesystem.
     * @return Filesystem::Type::AtaRaw.
     */
    Type type() const override { return Type::AtaRaw; } // Using a new type for raw ATA FS

    /**
     * @brief Returns the VNode operations table for this filesystem.
     * @return Pointer to the static VNodeOps table.
     */
    VNodeOps* get_vnode_ops() override { return &s_ata_raw_vnode_ops; }

    /**
     * @brief Probes a block device to determine if it contains an AtaFs.
     * For AtaFs, this probe always succeeds, treating any partition as a raw AtaFs.
     *
     * @param device The partition block device to probe.
     * @param first_sector The starting sector of the potential filesystem.
     * @return An Optional containing an OwnPtr to a new AtaFs instance on success,
     *         or an empty Optional if the probe fails (which it won't for AtaFs).
     */
    static fk::memory::optional<fk::memory::OwnPtr<::fkernel::fs::Filesystem>> probe(
        fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice> device,
        uint32_t first_sector);

    /**
     * @brief Registers the AtaFs driver with the VFS during early initialization.
     */
    static void early_register();

    // VNodeOps static functions
    static int ata_raw_read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size, size_t offset);
    static int ata_raw_write(VNode *vnode, FileDescriptor *fd, const void *buffer, size_t size, size_t offset);
    static int ata_raw_open(VNode *vnode, FileDescriptor *fd, int flags);
    static int ata_raw_close(VNode *vnode, FileDescriptor *fd);
    static int ata_raw_lookup(VNode *vnode, FileDescriptor *fd, const char *name, fk::memory::RetainPtr<VNode> &out);
    static int ata_raw_create(VNode *vnode, FileDescriptor *fd, const char *name, VNodeType type, fk::memory::RetainPtr<VNode> &out);
    static int ata_raw_readdir(VNode *vnode, FileDescriptor *fd, void *buffer, size_t max_entries);
    static int ata_raw_unlink(VNode *vnode, FileDescriptor *fd, const char *name);

    /**
     * @brief Static VNodeOps table for AtaFs.
     */
    static VNodeOps s_ata_raw_vnode_ops;

private:
    fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice> m_block_device;
    uint32_t m_first_sector;
    fk::memory::RetainPtr<VNode> m_root_vnode; ///< The root VNode for this filesystem instance.
};

} // namespace fkernel::drivers::ata
