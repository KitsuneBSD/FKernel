#pragma once

#include <Kernel/Block/BlockDevice.h>
#include <Kernel/Block/PartitionLocation.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <Kernel/FileSystem/file_descriptor.h>
#include <LibFK/Memory/retain_ptr.h>

namespace fkernel::block {

class PartitionBlockDevice final : public BlockDevice {
public:
  PartitionBlockDevice(fk::memory::RetainPtr<BlockDevice> device,
                       PartitionLocation &&location);

  int open(VNode *vnode, FileDescriptor *file_descriptor, int flags) override;
  int close(VNode *vnode, FileDescriptor *file_descriptor) override;
  int read(VNode *vnode, FileDescriptor *file_descriptor, void *buffer,
           size_t size, size_t offset) const override;
  int write(VNode *vnode, FileDescriptor *file_descriptor, const void *buffer,
            size_t size, size_t offset) override;

  int read_sectors(uint32_t lba, uint8_t sector_count,
                   void *buffer) const override;
  int write_sectors(uint32_t lba, uint8_t sector_count,
                    const void *buffer) override;

  /**
   * @brief Returns the size of the partition in bytes.
   *
   * @return The total size of the partition in bytes.
   */
  size_t size() const { return m_location.size_in_bytes(); }

  /**
   * @brief Returns the starting sector (LBA) of the partition.
   *
   * @return The first LBA of the partition.
   */
  uint32_t first_sector() const { return m_location.first_lba(); }

private:
  size_t adjust_and_get_final_offset(size_t &size, size_t offset) const;

  fk::memory::RetainPtr<BlockDevice> m_device;
  PartitionLocation m_location;
};

} // namespace fkernel::block
