#pragma once

#include <Kernel/Block/BlockDevice.h>
#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/FileSystem/file_descriptor.h>
#include <Kernel/Driver/Ata/AtaDefs.h>
#include <Kernel/Driver/Ata/AtaIoStrategy.h> // For fkernel::drivers::ata::AtaIoStrategy and AtaDeviceIoInfo

/**
 * @brief Block device interface for ATA devices
 *
 * Provides methods compatible with the Virtual File System (VFS)
 * to perform operations on a physical ATA device, including open, close,
 * read, and write, utilizing a pluggable I/O strategy.
 */
class AtaBlockDevice : public BlockDevice {
public:
  AtaBlockDevice(fkernel::drivers::ata::AtaDeviceIoInfo device_io_info,
                 fk::memory::OwnPtr<fkernel::drivers::ata::AtaIoStrategy> io_strategy);

  int open(VNode *vnode, FileDescriptor *fd, int flags) override;
  int close(VNode *vnode, FileDescriptor *fd) override;
  int read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size,
           size_t offset) const override;
  int write(VNode *vnode, FileDescriptor *fd, const void *buffer, size_t size,
            size_t offset) override;

  int read_sectors(uint32_t lba, uint8_t sector_count, void* buffer) const override;
  int write_sectors(uint32_t lba, uint8_t sector_count, const void* buffer) override;

private:
  fkernel::drivers::ata::AtaDeviceIoInfo m_device_io_info;
  fk::memory::OwnPtr<fkernel::drivers::ata::AtaIoStrategy> m_io_strategy;
};
