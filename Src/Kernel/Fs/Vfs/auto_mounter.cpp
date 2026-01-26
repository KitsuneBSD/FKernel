#include <Kernel/Fs/Vfs/auto_mounter.h>
#include <Kernel/Fs/Fat12/fat_12_fs.h>
#include <Kernel/Fs/Fat16/fat_16_fs.h>
#include <Kernel/Fs/Fat32/fat_32_fs.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {

void AutoMounter::try_mount(fk::RefPtr<StorageDevice> device) {
    if (!device) {
        fk::algorithms::kwarn("AUTO-MOUNT", "Attempted to mount null device");
        return;
    }
    
    fk::algorithms::klog("AUTO-MOUNT", "Attempting to auto-mount %s...", device->name().c_str());
    
    const char* device_name = device->name().c_str();
    if (!device_name) {
        fk::algorithms::kwarn("AUTO-MOUNT", "Device has null name");
        return;
    }
    
    char mount_path[64];
    snprintf(mount_path, sizeof(mount_path), "/mnt/%s", device_name);
    
    // Ensure mount directory exists
    auto mkdir_res = VirtualFileSystem::the().mkdir("/mnt", 0755);
    if (mkdir_res.is_error()) {
        // If /mnt doesn't exist, skip auto-mounting for now
        // This can happen during early boot before VFS is fully initialized
        return;
    }

    // Try FAT12
    auto fat12_res = Fat12FileSystem::create(device);
    if (fat12_res.is_ok()) {
        auto mount_res = VirtualFileSystem::the().mount(mount_path, fat12_res.value());
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO-MOUNT", "Mounted %s (FAT12) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO-MOUNT", "Failed to mount %s as FAT12: %s", device->name().c_str(), mount_path);
        }
    }

// Try FAT16
    auto fat16_res = Fat16FileSystem::create(device);
    if (fat16_res.is_ok()) {
        auto mount_res = VirtualFileSystem::the().mount(mount_path, fat16_res.value());
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO-MOUNT", "Mounted %s (FAT16) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO-MOUNT", "Failed to mount %s as FAT16: %s", device->name().c_str(), mount_path);
        }
    }

    // Try FAT32
    auto fat32_res = Fat32FileSystem::create(device);
    if (fat32_res.is_ok()) {
        auto mount_res = VirtualFileSystem::the().mount(mount_path, fat32_res.value());
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO-MOUNT", "Mounted %s (FAT32) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO-MOUNT", "Failed to mount %s as FAT32: %s", device->name().c_str(), mount_path);
        }
    }

    fk::algorithms::klog("AUTO-MOUNT", "No supported filesystem found on %s", device->name().c_str());
}
}
