#pragma once

#include <LibFK/Memory/retain_ptr.h>
#include <LibFK/Types/types.h>

#include <Kernel/Block/PartitionDevice.h>
#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>

struct FileDescriptor;

namespace fkernel::fs {

/**
 * @brief Abstract base class for all file systems.
 *
 * Defines the common interface for file systems, allowing for polymorphic
 * handling of different file system types (e.g., FAT, Ext2, RamFS).
 */
#include <LibFK/Container/static_vector.h>
#include <LibFK/Memory/optional.h>
#include <LibFK/Memory/own_ptr.h>

class Filesystem {
public:
  enum class Type {
    Unknown,
    FAT,
    RamFS,
    DevFS,
    AtaRaw, // Added for raw ATA partition access
  };

  virtual ~Filesystem() = default;

  /**
   * @brief Initializes the file system.
   *
   * This method performs any necessary setup for the file system, such as
   * reading superblock information.
   * @return 0 on success, negative error code on failure.
   */
  virtual int initialize() = 0;

  /**
   * @brief Returns the root VNode of the file system.
   *
   * @return A RetainPtr to the root VNode.
   */
  virtual fk::memory::RetainPtr<VNode> root_vnode() = 0;

  /**
   * @brief Returns the type of the file system.
   *
   * @return The Filesystem::Type enum value.
   */
  virtual Type type() const = 0;

  // Each filesystem will provide its specific VNodeOps table.
  virtual VNodeOps *get_vnode_ops() = 0;

  // Type for a static factory method that attempts to probe and create a
  // filesystem
  using ProbeFunction = fk::memory::optional<fk::memory::OwnPtr<Filesystem>> (
          *)(fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice> device,
             uint32_t first_sector);

  // Global registry for filesystem probe functions
  static fk::containers::static_vector<ProbeFunction, 8> s_filesystem_drivers;

  static void register_filesystem_driver(ProbeFunction probe_func) {
    if (s_filesystem_drivers.size() < s_filesystem_drivers.capacity()) {
      s_filesystem_drivers.push_back(probe_func);
      fk::algorithms::kdebug("FS_MANAGER",
                             "Registered new filesystem driver. Total: %zu",
                             s_filesystem_drivers.size());
    } else {
      fk::algorithms::kwarn(
          "FS_MANAGER",
          "Filesystem driver registration failed: capacity full.");
    }
  }
};

} // namespace fkernel::fs
