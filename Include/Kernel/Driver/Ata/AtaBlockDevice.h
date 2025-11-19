#pragma once

#include <Kernel/Block/BlockDevice.h>
#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/FileSystem/file_descriptor.h>
#include <Kernel/Driver/Ata/AtaDefs.h>

/**
 * @brief Block device interface for ATA devices
 *
 * Provides methods compatible with the Virtual File System (VFS)
 * to perform operations on a physical ATA device, including open, close,
 * read, and write.
 */
class AtaBlockDevice : public BlockDevice {
public:
  explicit AtaBlockDevice(const AtaDeviceInfo& device_info); // Constructor to initialize m_device_info

  int open(VNode *vnode, FileDescriptor *fd, int flags) override;

  int close(VNode *vnode, FileDescriptor *fd) override;

  int read(VNode *vnode, FileDescriptor *fd, void *buffer, size_t size,
           size_t offset) const override;

  int write(VNode *vnode, FileDescriptor *fd, const void *buffer, size_t size,
            size_t offset) override;

private:
  AtaDeviceInfo m_device_info; // Store device info directly
};
