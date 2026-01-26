#include <Kernel/Driver/Storage/Partitions/Gpt/gpt.h>
#include <Kernel/Driver/Storage/Partitions/Mbr/mbr.h>
#include <Kernel/Driver/Storage/Partitions/partition_manager.h>
#include <Kernel/Driver/Storage/storage_device.h>
#include <Kernel/Fs/DevFs/dev_fs.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/ref_ptr.h>

PartitionManager &PartitionManager::the() {
  static PartitionManager instance;
  return instance;
}

#include <Kernel/Fs/Vfs/auto_mounter.h>

void PartitionManager::add_partition(
    fk::RefPtr<Partition> partition) {
  if (!partition) {
    fk::algorithms::kwarn("PARTITION", "Attempted to add null partition");
    return;
  }
  
  m_partitions.add(partition);

  // Register in DevFs (BSD style: ad0s1, etc.)
  // Copy the name to ensure lifetime during registration
  auto* node_ptr = partition.get();
  if (node_ptr) {
    fk::text::String device_name = partition->name();  // Copy to ensure lifetime
    const char* name = device_name.c_str();
    if (name) {
      // Create a RefPtr from the RetainPtr to maintain proper lifetime
      fk::RefPtr<Node> node_ref = fk::adopt_ref<Node>(node_ptr);
      fkernel::DevFs::the().register_device(node_ref, name);
    }
  }

  fk::algorithms::klog("PARTITION", "Added LBA %llu, count %llu",
                       partition->start_sector(),
                       partition->sector_count().value());

  // Automatically try to mount the partition
  fkernel::AutoMounter::try_mount(partition);
}

bool PartitionManager::has_partitions_for_device(fk::RefPtr<StorageDevice> device) const {
  for (auto& partition : m_partitions.all()) {
    if (partition->underlying_device() == device) {
      return true;
    }
  }
  return false;
}

void PartitionManager::scan(fk::RefPtr<StorageDevice> device) {
  fk::algorithms::klog("PARTITION MANAGER", "[%s] Scanning for partitions...",
                       device->name().c_str());

  // Try GPT first
  if (GPTParser::parse(device).is_ok()) {
    fk::algorithms::klog("PARTITION MANAGER", "[%s] GPT partitioning detected",
                         device->name().c_str());
    return;
  }

  // Fallback to MBR
  if (MBRParser::parse(device).is_ok()) {
    fk::algorithms::klog("PARTITION MANAGER", "[%s] MBR partitioning detected",
                         device->name().c_str());
    return;
  }

  fk::algorithms::kwarn("PARTITION MANAGER",
                        "[%s] No valid partition table found",
                        device->name().c_str());
}
