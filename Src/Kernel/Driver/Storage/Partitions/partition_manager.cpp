#include <Kernel/Driver/Storage/Partitions/Gpt/gpt.h>
#include <Kernel/Driver/Storage/Partitions/Mbr/mbr.h>
#include <Kernel/Driver/Storage/Partitions/partition_manager.h>
#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Driver/Storage/Interfaces/storage_device.h>
#include <Kernel/Fs/Virtual/DevFs/dev_fs.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>

PartitionManager &PartitionManager::the() {
  static PartitionManager instance;
  return instance;
}

void PartitionManager::initialize() {
  fk::algorithms::klog("PARTITION", "Partition manager initialized");
  m_is_initialized = true;
}

#include <Kernel/Fs/Vfs/Mount/auto_mounter.h>

void PartitionManager::add_partition(
    fk::RefPtr<Partition> partition) {
  if (!partition) {
    fk::algorithms::kwarn("PARTITION", "Attempted to add null partition");
    return;
  }
  
  m_partitions.add(partition);

  // Register in DriverManager (which handles DevFs)
  fkernel::DriverManager::the().register_device(partition);

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
  fk::algorithms::klog("PARTITION_MANAGER", "[%s] Scanning for partitions...",
                       device->name().c_str());

  // Try GPT first
  if (GPTParser::parse(device).is_ok()) {
    fk::algorithms::klog("PARTITION_MANAGER", "[%s] GPT partitioning detected",
                         device->name().c_str());
    return;
  }

  // Fallback to MBR
  if (MBRParser::parse(device).is_ok()) {
    fk::algorithms::klog("PARTITION_MANAGER", "[%s] MBR partitioning detected",
                         device->name().c_str());
    return;
  }

  fk::algorithms::kwarn("PARTITION_MANAGER",
                        "[%s] No valid partition table found",
                        device->name().c_str());
}
