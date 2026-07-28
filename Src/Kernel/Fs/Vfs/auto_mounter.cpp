#include <Kernel/Fs/Vfs/auto_mounter.h>
#include <Kernel/Driver/Storage/Partitions/partition_manager.h>
#include <Kernel/Fs/Disk/Fat12/fat_12_fs.h>
#include <Kernel/Fs/Disk/Fat16/fat_16_fs.h>
#include <Kernel/Fs/Disk/Fat32/fat_32_fs.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {

void AutoMounter::try_mount(fk::RefPtr<StorageDevice> device) {
    if (!device) {
        fk::algorithms::kwarn("AUTO_MOUNT", "Attempted to mount null device");
        return;
    }
    
    fk::algorithms::klog("AUTO_MOUNT", "Attempting to auto-mount %s...", device->name().c_str());
    
    const char* device_name = device->name().c_str();
    if (!device_name) {
        fk::algorithms::kwarn("AUTO_MOUNT", "Device has null name");
        return;
    }
    
    char mount_path[64];
    snprintf(mount_path, sizeof(mount_path), "/mnt/%s", device_name);
    
    // Ensure mount directory exists
    auto mkdir_res = fkernel::VirtualFileSystem::the().mkdir("/mnt", 0755);
    if (mkdir_res.is_error() && mkdir_res.error() != fk::core::Error::PermissionDenied) {
        // If it's not a "PermissionDenied" (which TmpFs uses for AlreadyExists),
        // we might have a real issue. But let's try to proceed anyway if /mnt exists.
        fk::algorithms::kwarn("AUTO_MOUNT", "mkdir /mnt failed: %d", (int)mkdir_res.error());
    }

    // Try FAT12
    auto fat12_res = Fat12FileSystem::create(device);
    if (fat12_res.is_ok()) {
        auto mount_res = fkernel::VirtualFileSystem::the().mount(mount_path, fat12_res.value(), "fat12");
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (FAT12) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO_MOUNT", "Failed to mount %s as FAT12: error=%d", device->name().c_str(), (int)mount_res.error());
        }
    }

// Try FAT16
    auto fat16_res = Fat16FileSystem::create(device);
    if (fat16_res.is_ok()) {
        auto mount_res = fkernel::VirtualFileSystem::the().mount(mount_path, fat16_res.value(), "fat16");
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (FAT16) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO_MOUNT", "Failed to mount %s as FAT16: error=%d", device->name().c_str(), (int)mount_res.error());
        }
    }

    // Try FAT32
    auto fat32_res = Fat32FileSystem::create(device);
    if (fat32_res.is_ok()) {
        auto mount_res = fkernel::VirtualFileSystem::the().mount(mount_path, fat32_res.value(), "fat32");
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (FAT32) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO_MOUNT", "Failed to mount %s as FAT32: error=%d", device->name().c_str(), (int)mount_res.error());
        }
    }

    fk::algorithms::klog("AUTO_MOUNT", "No supported filesystem found on %s", device->name().c_str());
}

void AutoMounter::mount_all_partitions() {
    const auto& partitions = PartitionManager::the().partitions().all();
    for (size_t i = 0; i < partitions.size(); ++i) {
        auto& part = partitions[i];
        if (!part) continue;
        try_mount(fk::RefPtr<StorageDevice>(part.ptr()));
    }
}

bool AutoMounter::try_mount_at(fk::RefPtr<StorageDevice> device, const char* target_path) {
    if (!device || !target_path) return false;

    auto fat12_res = Fat12FileSystem::create(device);
    if (fat12_res.is_ok()) {
        if (VirtualFileSystem::the().mount(target_path, fat12_res.value(), "fat12").is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (FAT12) at %s", device->name().c_str(), target_path);
            return true;
        }
    }

    auto fat16_res = Fat16FileSystem::create(device);
    if (fat16_res.is_ok()) {
        if (VirtualFileSystem::the().mount(target_path, fat16_res.value(), "fat16").is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (FAT16) at %s", device->name().c_str(), target_path);
            return true;
        }
    }

    auto fat32_res = Fat32FileSystem::create(device);
    if (fat32_res.is_ok()) {
        if (VirtualFileSystem::the().mount(target_path, fat32_res.value(), "fat32").is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (FAT32) at %s", device->name().c_str(), target_path);
            return true;
        }
    }

    fk::algorithms::kwarn("AUTO_MOUNT", "No supported filesystem found on %s for mount at %s",
                           device->name().c_str(), target_path);
    return false;
}

}
