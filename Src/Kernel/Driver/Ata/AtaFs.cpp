#include <Kernel/Driver/Ata/AtaFs.h>
#include <LibFK/Algorithms/log.h>

#include <LibFK/Memory/own_ptr.h>
#include <LibC/string.h> // For memset and strcmp

namespace fkernel::drivers::ata {

// Static VNodeOps table definition
VNodeOps AtaFs::s_ata_raw_vnode_ops = {
    .read = &AtaFs::ata_raw_read,
    .write = &AtaFs::ata_raw_write,
    .open = &AtaFs::ata_raw_open,
    .close = &AtaFs::ata_raw_close,
    .lookup = &AtaFs::ata_raw_lookup,
    .create = &AtaFs::ata_raw_create,
    .readdir = &AtaFs::ata_raw_readdir,
    .unlink = &AtaFs::ata_raw_unlink,
};

AtaFs::AtaFs(fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice> block_device,
             uint32_t first_sector)
    : m_block_device(block_device)
    , m_first_sector(first_sector) {
    fk::algorithms::klog("ATA FS", "AtaFs constructed for first sector %u.", first_sector);
    // Initialize m_root_vnode to nullptr, will be set in initialize()
    m_root_vnode = nullptr;
}

int AtaFs::initialize() {
    fk::algorithms::klog("ATA FS", "Initializing AtaFs at LBA %u.", m_first_sector);

    auto ata_raw_root_vnode = fk::memory::adopt_retain(new VNode());
    ata_raw_root_vnode->m_name = "/"; // Root of this raw filesystem
    ata_raw_root_vnode->type = VNodeType::BlockDevice; // Represents the raw partition
    ata_raw_root_vnode->ops = &AtaFs::s_ata_raw_vnode_ops; // Use AtaFs VNode ops
    ata_raw_root_vnode->fs_private = this; // Point back to this AtaFs instance
    ata_raw_root_vnode->inode_number = 0; // No meaningful inode for a raw disk
    ata_raw_root_vnode->size = m_block_device->size(); // Size of the raw partition
    ata_raw_root_vnode->inode = new Inode(0); // Dummy inode

    m_root_vnode = ata_raw_root_vnode;
    fk::algorithms::klog("ATA FS", "AtaFs root VNode initialized, size %llu.", ata_raw_root_vnode->size);

    return 0;
}

fk::memory::RetainPtr<VNode> AtaFs::root_vnode() {
    return m_root_vnode;
}

fk::memory::optional<fk::memory::OwnPtr<::fkernel::fs::Filesystem>> AtaFs::probe(
    fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice> device,
    uint32_t first_sector) {
    fk::algorithms::klog("ATA FS", "Probing for AtaFs on LBA %u. This probe always succeeds for raw disk access.",
                         first_sector);

    // AtaFs acts as a wrapper for raw block device access.
    // It can always "probe" successfully as long as there's a block device.
    return fk::memory::optional<fk::memory::OwnPtr<::fkernel::fs::Filesystem>>(
        fk::memory::adopt_own<::fkernel::fs::Filesystem>(
            new AtaFs(fk::types::move(device), first_sector)));
}

void AtaFs::early_register() {
    fk::algorithms::klog("ATA FS", "Registering AtaFs driver (early).");
    Filesystem::register_filesystem_driver(AtaFs::probe);
}

// VNodeOps Implementations

static inline AtaFs *get_ata_fs(VNode *vnode) {
  if (!vnode || !vnode->fs_private) {
    fk::algorithms::kerror("ATA FS VNODE", "VNode or fs_private is null.");
    return nullptr;
  }
  return reinterpret_cast<AtaFs *>(vnode->fs_private);
}

int AtaFs::ata_raw_read(VNode *vnode, [[maybe_unused]] FileDescriptor *fd,
                        void *buffer, size_t size, size_t offset) {
  fk::algorithms::klog("ATA FS VNODE",
                       "ata_raw_read for '%s', size %zu, offset %zu",
                       vnode->m_name.c_str(), size, offset);
  AtaFs *ata_fs = get_ata_fs(vnode);
  if (!ata_fs || !ata_fs->m_block_device) {
    fk::algorithms::kerror("ATA FS VNODE", "AtaFs instance or block device is null.");
    return -1;
  }
  // Delegate directly to the underlying block device's read, adjusting for first_sector offset.
  // Note: VNode::read expects a byte offset, so we need to convert first_sector (LBA) to bytes.
  return ata_fs->m_block_device->read(vnode, fd, buffer, size, ata_fs->m_first_sector * 512 + offset);
}

int AtaFs::ata_raw_write(VNode *vnode, [[maybe_unused]] FileDescriptor *fd,
                         const void *buffer, size_t size, size_t offset) {
  fk::algorithms::klog("ATA FS VNODE",
                       "ata_raw_write for '%s', size %zu, offset %zu",
                       vnode->m_name.c_str(), size, offset);
  AtaFs *ata_fs = get_ata_fs(vnode);
  if (!ata_fs || !ata_fs->m_block_device) {
    fk::algorithms::kerror("ATA FS VNODE", "AtaFs instance or block device is null.");
    return -1;
  }
  // Delegate directly to the underlying block device's write, adjusting for first_sector offset.
  return ata_fs->m_block_device->write(vnode, fd, buffer, size, ata_fs->m_first_sector * 512 + offset);
}

int AtaFs::ata_raw_open(VNode *vnode, FileDescriptor *fd, int flags) {
  (void)vnode;
  (void)fd;
  (void)flags;
  fk::algorithms::klog("ATA FS VNODE", "ata_raw_open called.");
  return 0; // Always succeed for raw open
}

int AtaFs::ata_raw_close(VNode *vnode, FileDescriptor *fd) {
  (void)vnode;
  (void)fd;
  fk::algorithms::klog("ATA FS VNODE", "ata_raw_close called.");
  return 0; // Always succeed for raw close
}

int AtaFs::ata_raw_lookup(VNode *vnode, [[maybe_unused]] FileDescriptor *fd,
                          const char *name, fk::memory::RetainPtr<VNode> &out) {
  (void)vnode;
  (void)fd;
  (void)name;
  (void)out;
  fk::algorithms::kwarn("ATA FS VNODE", "ata_raw_lookup not supported for raw AtaFs.");
  return -1; // Not supported
}

int AtaFs::ata_raw_create(VNode *vnode, [[maybe_unused]] FileDescriptor *fd,
                          const char *name, VNodeType type, fk::memory::RetainPtr<VNode> &out) {
  (void)vnode;
  (void)fd;
  (void)name;
  (void)type;
  (void)out;
  fk::algorithms::kwarn("ATA FS VNODE", "ata_raw_create not supported for raw AtaFs.");
  return -1; // Not supported
}

int AtaFs::ata_raw_readdir(VNode *vnode, [[maybe_unused]] FileDescriptor *fd,
                           void *buffer, size_t max_entries) {
  (void)vnode;
  (void)fd;
  (void)buffer;
  (void)max_entries;
  fk::algorithms::kwarn("ATA FS VNODE", "ata_raw_readdir not supported for raw AtaFs.");
  return -1; // Not supported
}

int AtaFs::ata_raw_unlink(VNode *vnode, [[maybe_unused]] FileDescriptor *fd, const char *name) {
  (void)vnode;
  (void)fd;
  (void)name;
  fk::algorithms::kwarn("ATA FS VNODE", "ata_raw_unlink not supported for raw AtaFs.");
  return -1; // Not supported
}

} // namespace fkernel::drivers::ata
