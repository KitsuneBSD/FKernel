#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Driver/Storage/Ata/ata_controller.h>
#include <Kernel/Driver/Storage/Ata/pio_strategy.h>
#include <Kernel/Driver/Storage/Partitions/partition_manager.h>
#include <Kernel/Driver/Storage/storage_device_name.h>
#include <Kernel/Fs/DevFs/dev_fs.h>
#include <Kernel/Fs/Vfs/auto_mounter.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Memory/own_ptr.h>

void ATAController::initialize() {
  fk::algorithms::klog("ATA CONTROLLER", "Controller initialized");
}

ATAController &ATAController::the() {
  static ATAController instance;
  return instance;
}

void ATAController::detect_devices() {
  fk::algorithms::klog("ATA CONTROLLER", "Starting device detection...");

  auto &pci_devices = PciManager::the().devices();
  for (auto &device : pci_devices) {
    if (device.class_code() == 0x01 && device.subclass_code() == 0x01) {
      detect_on_pci(device);
    }
  }

  detect_legacy();

  fk::algorithms::klog("ATA CONTROLLER",
                       "Detection complete. Found %d devices.",
                       m_devices.size());
}

void ATAController::detect_on_pci(const PciDevice &device) {
  fk::algorithms::klog("ATA CONTROLLER",
                       "Found PCI IDE controller at %02x:%02x.%d",
                       device.address().bus(), device.address().device(),
                       device.address().function());
}

void ATAController::detect_legacy() {
  uint16_t bases[] = {0x1F0, 0x170};
  uint16_t controls[] = {0x3F6, 0x376};

  for (int i = 0; i < 2; ++i) {
    for (int master = 0; master < 2; ++master) {
      uint16_t io = bases[i];
      uint16_t ctrl = controls[i];
      bool is_master = (master == 0);

      fk::algorithms::klog("ATA CONTROLLER",
                           "Probing %s on channel %d (IO: %04x)...",
                           is_master ? "Master" : "Slave", i, io);

      // IDENTIFY command
      outb(io + 6, is_master ? 0xA0 : 0xB0);
      outb(io + 2, 0);
      outb(io + 3, 0);
      outb(io + 4, 0);
      outb(io + 5, 0);
      outb(io + 7, 0xEC); // IDENTIFY

      uint8_t status = inb(io + 7);
      if (status == 0)
        continue; // No device

      while (inb(io + 7) & 0x80)
        ; // Wait BSY

      if (inb(io + 4) != 0 || inb(io + 5) != 0)
        continue; // Not ATA

      while (!(inb(io + 7) & 0x08))
        ; // Wait DRQ

      uint16_t data[256];
      for (int j = 0; j < 256; ++j) {
        data[j] = inw(io);
      }

      uint64_t sectors = 0;
      if (data[83] & (1 << 10)) { // LBA48 support
        // Bounds checking before accessing array
        if (data[100] == 0 && data[101] == 0 && data[102] == 0 && data[103] == 0) {
          fk::algorithms::kwarn("ATA", "Invalid LBA48 sector count - all zeros");
          continue;
        }
        sectors = (uint64_t)data[100] | ((uint64_t)data[101] << 16) |
                  ((uint64_t)data[102] << 32) | ((uint64_t)data[103] << 48);
      } else {
        // Add bounds checking for pointer cast
        if (data[60] == 0 && data[61] == 0) {
          fk::algorithms::kwarn("ATA", "Invalid LBA28 sector count - all zeros");
          continue;
        }
        sectors = (uint64_t)data[60] | ((uint64_t)data[61] << 16);
      }

      // Validate sector count with reasonable bounds
      if (sectors == 0 || sectors > 0x100000000ULL) {
        fk::algorithms::kwarn("ATA", "Invalid sector count: %llu", sectors);
        continue;
      }

      ASSERT(sectors > 0);

      uint64_t total_bytes = sectors * 512;
      uint64_t mib = total_bytes / (1024 * 1024);
      uint64_t gib = mib / 1024;

      fk::text::String bsd_name = StorageDeviceName::bsd_ata(m_devices.size());

      fk::algorithms::klog("ATA", "Detected drive [%s]. Sectors: %lu (%lu GiB)",
                           bsd_name.c_str(), sectors, gib);

      auto strategy =
          fk::memory::make_owned<PIOStrategy>(io, ctrl, is_master);
      if (!strategy)
        continue;

      // Properly convert OwnPtr<PIOStrategy> to OwnPtr<ATATransferStrategy>
      // PIOStrategy inherits from ATATransferStrategy
      fk::memory::OwnPtr<ATATransferStrategy> transfer_strategy(
          strategy.leak_ptr());
      
      auto dev_res = fk::make_ref<ATADevice>(
          bsd_name, fk::types::move(transfer_strategy), SectorCount(sectors));
      if (dev_res.is_error())
        continue;
      auto dev = fk::types::move(dev_res.value());

      if (sectors == 0) {
          fk::algorithms::kwarn("ATA", "[%s] Disk has 0 sectors, skipping.", bsd_name.c_str());
          continue;
      }

      m_devices.push_back(dev);

      // Register in DevFs with proper name lifetime management
      fk::text::String device_name = bsd_name;  // Copy to ensure lifetime
      fk::RefPtr<Node> node_ref = dev;
      fkernel::DevFs::the().register_device(node_ref, device_name.c_str());

      // Scan for partitions
      PartitionManager::the().scan(dev);

      // Try to mount the raw device (Superfloppy) only if NO partitions were found
      if (!PartitionManager::the().has_partitions_for_device(dev)) {
          fk::algorithms::klog("ATA", "[%s] No partitions found, trying raw mount...", bsd_name.c_str());
          fkernel::AutoMounter::try_mount(dev);
      } else {
          fk::algorithms::klog("ATA", "[%s] Partitions found, skipping raw mount.", bsd_name.c_str());
      }
    }
  }
}
