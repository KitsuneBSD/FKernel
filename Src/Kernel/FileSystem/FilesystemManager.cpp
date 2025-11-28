#include <Kernel/FileSystem/FilesystemManager.h>
#include <LibFK/Algorithms/log.h>

#include <LibC/string.h> // For snprintf

#include <Kernel/FileSystem/DevFS/devfs.h> // For registering partition devices in DevFS

namespace fkernel::fs {

FilesystemManager &FilesystemManager::the() {
  static FilesystemManager instance;
  return instance;
}

void FilesystemManager::initialize() {
  fk::algorithms::klog("FS MANAGER", "Initializing FilesystemManager...");

  // Get block devices from AtaController
  auto &ata_devices = AtaController::the().devices();

  char device_name_buffer[16];
  int ata_device_index = 0;

  for (const auto &block_device_ptr : ata_devices) {
    if (!block_device_ptr) {
      fk::algorithms::kwarn("FS MANAGER",
                            "Skipping null block device pointer.");
      continue;
    }

    // Register the raw block device (e.g., /dev/ada0)
    snprintf(device_name_buffer, sizeof(device_name_buffer), "ada%d",
             ata_device_index);
    fk::algorithms::klog("FS MANAGER", "Registering raw block device /dev/%s.",
                         device_name_buffer);
    DevFS::the().register_device(
        device_name_buffer, VNodeType::BlockDevice, &g_block_device_ops,
        static_cast<void *>(const_cast<BlockDevice *>(block_device_ptr.get())));

    fk::algorithms::klog("FS MANAGER", "Detecting partitions on /dev/%s.",
                         device_name_buffer);
    PartitionManager partition_manager(block_device_ptr);
    auto partitions = partition_manager.detect_partitions();

    char partition_name_buffer[32];
    int partition_index = 0;

    for (auto &partition_device_ptr : partitions) {
      if (!partition_device_ptr) {
        fk::algorithms::kwarn("FS MANAGER",
                              "Skipping null partition device pointer.");
        continue;
      }

      // Register the partition device (e.g., /dev/ada0p1)
      snprintf(partition_name_buffer, sizeof(partition_name_buffer), "%sp%d",
               device_name_buffer, partition_index + 1);
      fk::algorithms::klog("FS MANAGER",
                           "Registering partition device /dev/%s.",
                           partition_name_buffer);
      DevFS::the().register_device(partition_name_buffer,
                                   VNodeType::BlockDevice, &g_block_device_ops,
                                   partition_device_ptr.get(), true, false);

      fk::algorithms::klog(
          "FS MANAGER",
          "Probing filesystems on partition /dev/%s (LBA %u, size %llu bytes)",
          partition_name_buffer, partition_device_ptr->first_sector(),
          partition_device_ptr->size());

      // Attempt to probe filesystems using registered drivers
      bool filesystem_mounted = false;
      for (auto &probe_func : Filesystem::s_filesystem_drivers) {
        auto optional_fs = probe_func(partition_device_ptr,
                                      partition_device_ptr->first_sector());
        if (optional_fs.has_value()) {
          fk::memory::OwnPtr<Filesystem> fs_own_ptr =
              fk::types::move(optional_fs.value());
          if (fs_own_ptr) {
            int init_res = fs_own_ptr->initialize();
            if (init_res == 0) {
              // Successfully initialized, now mount it
              fk::memory::RetainPtr<Filesystem> fs_retain_ptr =
                  fk::memory::adopt_retain(fs_own_ptr.leakPtr());

              // Determine mount path: /<device_name_buffer>p<partition_index+1>
              fk::text::fixed_string<256> mount_path;
              mount_path.append("/");
              mount_path.append(partition_name_buffer);

              VirtualFS::the().mount(mount_path.c_str(),
                                     fs_retain_ptr->root_vnode());
              m_mounted_filesystems.push_back(fs_retain_ptr);
              filesystem_mounted = true;
              fk::algorithms::klog("FS MANAGER",
                                   "Mounted filesystem type %d on /%s.",
                                   static_cast<int>(fs_retain_ptr->type()),
                                   partition_name_buffer);
              break; // Only one filesystem per partition
            } else {
              fk::algorithms::kerror(
                  "FS MANAGER",
                  "Failed to initialize filesystem on /%s (type %d), error %d.",
                  partition_name_buffer, static_cast<int>(fs_own_ptr->type()),
                  init_res);
            }
          }
        }
      }
      if (!filesystem_mounted) {
        fk::algorithms::kwarn("FS MANAGER",
                              "No known filesystem detected or mounted on /%s.",
                              partition_name_buffer);
      }
      partition_index++;
    }
    ata_device_index++;
  }
  fk::algorithms::klog("FS MANAGER", "Total mounted filesystems : %zu.",
                       m_mounted_filesystems.count);
}

} // namespace fkernel::fs
