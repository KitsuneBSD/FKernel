#include <Kernel/Fs/Vfs/auto_mounter.h>
#include <Kernel/Driver/Storage/Partitions/partition_manager.h>
#include <Kernel/Fs/Disk/Fat12/fat_12_fs.h>
#include <Kernel/Fs/Disk/Fat16/fat_16_fs.h>
#include <Kernel/Fs/Disk/Fat32/fat_32_fs.h>
#include <Kernel/Fs/Disk/MinixFs/minix_fs.h>
#include <Kernel/Fs/Disk/Ext2/ext2_fs.h>
#include <Kernel/Fs/Disk/Iso9660/iso9660_fs.h>
#include <Kernel/Fs/Disk/Exfat/exfat_fs.h>
#include <Kernel/Fs/Disk/Ext3/ext3_fs.h>
#include <Kernel/Fs/Disk/Ext4/ext4_fs.h>
#include <Kernel/Fs/Disk/Ufs/ufs_fs.h>
#include <Kernel/Fs/Disk/HfsPlus/hfsplus_fs.h>
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

    // Try ExFAT (before ext2: ExFAT magic is distinct from FAT32)
    auto exfat_res = ExfatFileSystem::create(device);
    if (exfat_res.is_ok()) {
        auto mount_res = fkernel::VirtualFileSystem::the().mount(mount_path, exfat_res.value(), "exfat");
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (ExFAT) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO_MOUNT", "Failed to mount %s as ExFAT: error=%d", device->name().c_str(), (int)mount_res.error());
        }
    }

    // Try ext4 (before ext3: ext4 requires EXTENTS incompat flag)
    auto ext4_res = Ext4FileSystem::create(device);
    if (ext4_res.is_ok()) {
        auto mount_res = fkernel::VirtualFileSystem::the().mount(mount_path, ext4_res.value(), "ext4");
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (ext4) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO_MOUNT", "Failed to mount %s as ext4: error=%d", device->name().c_str(), (int)mount_res.error());
        }
    }

    // Try ext3 (before ext2: ext3 accepts HAS_JOURNAL; ext2 rejects INCOMPAT_RECOVER)
    auto ext3_res = Ext3FileSystem::create(device);
    if (ext3_res.is_ok()) {
        auto mount_res = fkernel::VirtualFileSystem::the().mount(mount_path, ext3_res.value(), "ext3");
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (ext3) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO_MOUNT", "Failed to mount %s as ext3: error=%d", device->name().c_str(), (int)mount_res.error());
        }
    }

    // Try ext2
    auto ext2_res = Ext2FileSystem::create(device);
    if (ext2_res.is_ok()) {
        auto mount_res = fkernel::VirtualFileSystem::the().mount(mount_path, ext2_res.value(), "ext2");
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (ext2) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO_MOUNT", "Failed to mount %s as ext2: error=%d", device->name().c_str(), (int)mount_res.error());
        }
    }

    // Try MinixFS
    auto minix_res = MinixFileSystem::create(device);
    if (minix_res.is_ok()) {
        auto mount_res = fkernel::VirtualFileSystem::the().mount(mount_path, minix_res.value(), "minix");
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (MinixFS) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO_MOUNT", "Failed to mount %s as MinixFS: error=%d", device->name().c_str(), (int)mount_res.error());
        }
    }

    // Try ISO9660
    auto iso_res = Iso9660FileSystem::create(device);
    if (iso_res.is_ok()) {
        auto mount_res = fkernel::VirtualFileSystem::the().mount(mount_path, iso_res.value(), "iso9660");
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (ISO9660) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO_MOUNT", "Failed to mount %s as ISO9660: error=%d", device->name().c_str(), (int)mount_res.error());
        }
    }

    // Try UFS/UFS2 (BSD native)
    auto ufs_res = UfsFileSystem::create(device);
    if (ufs_res.is_ok()) {
        auto mount_res = fkernel::VirtualFileSystem::the().mount(mount_path, ufs_res.value(), "ufs");
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (UFS) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO_MOUNT", "Failed to mount %s as UFS: error=%d", device->name().c_str(), (int)mount_res.error());
        }
    }

    // Try HFS+ / HFSX (macOS native)
    auto hfsplus_res = HFSPlusFileSystem::create(device);
    if (hfsplus_res.is_ok()) {
        auto mount_res = fkernel::VirtualFileSystem::the().mount(mount_path, hfsplus_res.value(), "hfsplus");
        if (mount_res.is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (HFS+) at %s", device->name().c_str(), mount_path);
            return;
        } else {
            fk::algorithms::kwarn("AUTO_MOUNT", "Failed to mount %s as HFS+: error=%d", device->name().c_str(), (int)mount_res.error());
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

    auto exfat_res2 = ExfatFileSystem::create(device);
    if (exfat_res2.is_ok()) {
        if (VirtualFileSystem::the().mount(target_path, exfat_res2.value(), "exfat").is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (ExFAT) at %s", device->name().c_str(), target_path);
            return true;
        }
    }

    auto ext4_res2 = Ext4FileSystem::create(device);
    if (ext4_res2.is_ok()) {
        if (VirtualFileSystem::the().mount(target_path, ext4_res2.value(), "ext4").is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (ext4) at %s", device->name().c_str(), target_path);
            return true;
        }
    }

    auto ext3_res2 = Ext3FileSystem::create(device);
    if (ext3_res2.is_ok()) {
        if (VirtualFileSystem::the().mount(target_path, ext3_res2.value(), "ext3").is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (ext3) at %s", device->name().c_str(), target_path);
            return true;
        }
    }

    auto ext2_res = Ext2FileSystem::create(device);
    if (ext2_res.is_ok()) {
        if (VirtualFileSystem::the().mount(target_path, ext2_res.value(), "ext2").is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (ext2) at %s", device->name().c_str(), target_path);
            return true;
        }
    }

    auto minix_res = MinixFileSystem::create(device);
    if (minix_res.is_ok()) {
        if (VirtualFileSystem::the().mount(target_path, minix_res.value(), "minix").is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (MinixFS) at %s", device->name().c_str(), target_path);
            return true;
        }
    }

    auto iso_res2 = Iso9660FileSystem::create(device);
    if (iso_res2.is_ok()) {
        if (VirtualFileSystem::the().mount(target_path, iso_res2.value(), "iso9660").is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (ISO9660) at %s", device->name().c_str(), target_path);
            return true;
        }
    }

    auto ufs_res2 = UfsFileSystem::create(device);
    if (ufs_res2.is_ok()) {
        if (VirtualFileSystem::the().mount(target_path, ufs_res2.value(), "ufs").is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (UFS) at %s", device->name().c_str(), target_path);
            return true;
        }
    }

    auto hfsplus_res2 = HFSPlusFileSystem::create(device);
    if (hfsplus_res2.is_ok()) {
        if (VirtualFileSystem::the().mount(target_path, hfsplus_res2.value(), "hfsplus").is_ok()) {
            fk::algorithms::klog("AUTO_MOUNT", "Mounted %s (HFS+) at %s", device->name().c_str(), target_path);
            return true;
        }
    }

    fk::algorithms::kwarn("AUTO_MOUNT", "No supported filesystem found on %s for mount at %s",
                           device->name().c_str(), target_path);
    return false;
}

}
