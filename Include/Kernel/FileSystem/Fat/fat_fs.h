#pragma once

#include <Kernel/Block/BlockDevice.h>
#include <Kernel/FileSystem/Fat/fat_defs.h>
#include <Kernel/FileSystem/VirtualFS/filesystem.h> // Include the new base class
#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Memory/retain_ptr.h>
#include <LibFK/Types/types.h>

namespace fkernel::block {
class PartitionBlockDevice;
}

#define MAX_SECTOR_SIZE 4096 // Max sector size for FAT (e.g., 512, 1024, 2048, 4096)

namespace fkernel::fs::fat {

enum class FatType { FAT12, FAT16, FAT32, Unknown };

struct FatFilePrivateData {
  uint32_t first_cluster;  ///< Starting cluster of the file/directory
  uint32_t current_offset; ///< Current read/write offset within the file
  uint32_t size;           ///< Size of the file/directory
};

/**
 * @brief FAT File System implementation.
 *
 * This class handles reading and writing to FAT12, FAT16, and FAT32
 * filesystems. It interacts with a BlockDevice for low-level sector I/O
 * and exposes VNodeOps for integration with the Virtual File System.
 */
class FatFileSystem : public ::fkernel::fs::Filesystem { // Inherit from Filesystem
public:
  /**
   * @brief Constructs a FatFileSystem instance.
   *
   * @param block_device The underlying block device.
   * @param first_sector The starting sector of this FAT filesystem on the
   * device.
   */
  explicit FatFileSystem(fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice> block_device,
                         uint32_t first_sector);

  /**
   * @brief Initializes the FAT filesystem.
   *
   * Reads the boot sector and determines the FAT type.
   * @return 0 on success, negative error code on failure.
   */
  int initialize() override; // Mark as override

  FatType get_fat_type() const { return m_fat_type; }
  
  fk::memory::RetainPtr<VNode> root_vnode() override; // Implement pure virtual function
  Type type() const override { return Type::FAT; }       // Implement pure virtual function
  VNodeOps* get_vnode_ops() override { return &s_fat_vnode_ops; } // Implement pure virtual function

  uint32_t get_root_cluster() const { return m_boot_sector.root_cluster; }

  uint32_t get_first_data_sector() const;
  uint32_t get_sectors_per_fat() const;
  uint32_t get_root_dir_sectors() const;
  uint32_t get_total_clusters() const;
  uint32_t get_first_fat_sector() const;
  uint32_t cluster_to_lba(uint32_t cluster) const;

  /**
   * @brief Reads a cluster from the disk.
   *
   * @param cluster The cluster number to read.
   * @param buffer The buffer to store the cluster data.
   * @return 0 on success, negative error code on failure.
   */
  int read_cluster(uint32_t cluster, void *buffer) const;

  /**
   * @brief Writes a cluster to the disk.
   *
   * @param cluster The cluster number to write.
   * @param buffer The buffer containing the cluster data.
   * @return 0 on success, negative error code on failure.
   */
  int write_cluster(uint32_t cluster, const void *buffer);

  /**
   * @brief Reads the next cluster in the FAT chain.
   *
   * @param current_cluster The current cluster number.
   * @return The next cluster number, or FAT_EOF/FAT_BAD_CLUSTER.
   */
  uint32_t get_next_cluster(uint32_t current_cluster) const;

  /**
   * @brief Sets the next cluster in the FAT chain.
   *
   * @param current_cluster The current cluster number.
   * @param next_cluster The next cluster number to set.
   * @return 0 on success, negative error code on failure.
   */
  int set_next_cluster(uint32_t current_cluster, uint32_t next_cluster);

  /**
   * @brief Finds a free cluster in the FAT.
   *
   * @return The first free cluster number, or 0 if none found.
   */
  uint32_t find_free_cluster() const;

  /**
   * @brief Allocates a new cluster and links it to the previous one.
   *
   * @param previous_cluster The cluster to link from.
   * @return The newly allocated cluster number, or 0 on failure.
   */
  uint32_t allocate_cluster(uint32_t previous_cluster);

  static fk::memory::optional<fk::memory::OwnPtr<Filesystem>> probe(
      fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice> device,
      uint32_t first_sector);

  // VNodeOps implementations
  static int fat_read(VNode *vnode, FileDescriptor *fd, void *buffer,
                      size_t size, size_t offset);
  static int fat_write(VNode *vnode, FileDescriptor *fd, const void *buffer,
                       size_t size, size_t offset);
  static int fat_open(VNode *vnode, FileDescriptor *fd, int flags);
  static int fat_close(VNode *vnode, FileDescriptor *fd);
  static int fat_lookup(VNode *vnode, FileDescriptor *fd, const char *name,
                        fk::memory::RetainPtr<VNode> &out);
  static int fat_create(VNode *vnode, FileDescriptor *fd, const char *name,
                        VNodeType type, fk::memory::RetainPtr<VNode> &out);
  static int fat_readdir(VNode *vnode, FileDescriptor *fd, void *buffer,
                         size_t max_entries);
  static int fat_unlink(VNode *vnode, FileDescriptor *fd, const char *name);

  static VNodeOps s_fat_vnode_ops;

private:
  fk::memory::RetainPtr<BlockDevice>
      m_block_device;                   ///< Underlying block device
  uint32_t m_first_sector;              ///< Start sector of the partition
  FatBootSector m_boot_sector;          ///< Parsed boot sector
  FatType m_fat_type{FatType::Unknown}; ///< Detected FAT type
  uint32_t m_fat_start_sector;          ///< First sector of the first FAT
  uint32_t m_root_dir_start_sector;     ///< First sector of the root directory
                                        ///< (FAT12/16)
  uint32_t m_data_start_sector;         ///< First sector of the data region
  uint32_t m_sectors_per_fat;           ///< Number of sectors per FAT
  uint32_t m_total_clusters; ///< Total number of clusters in the FAT area
  
  fk::memory::RetainPtr<VNode> m_root_vnode; ///< The root VNode for this filesystem instance.

  // Helper to read the boot sector
  int read_boot_sector(FatBootSector *boot_sector) const;

  // Helper to determine FAT type
  FatType determine_fat_type() const;

  // Helper to convert short filename to string
  static void fat_name_to_string(const char filename[8],
                                 const char extension[3],
                                 fk::text::fixed_string<256> &out_name);
  // Helper to convert string to short filename
  static void string_to_fat_name(const char *in_name, char filename[8],
                                 char extension[3]);
};

} // namespace fkernel::fs::fat
