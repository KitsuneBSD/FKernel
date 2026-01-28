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

  bool found_pci_ide = false;
  auto &pci_devices = PciManager::the().devices();
  for (auto &device : pci_devices) {
    if (device.class_code() == 0x01 && device.subclass_code() == 0x01) {
      detect_on_pci(device);
      found_pci_ide = true;
    }
  }

  if (!found_pci_ide) {
    fk::algorithms::klog("ATA CONTROLLER", "No PCI IDE controller found, falling back to legacy probing");
    detect_legacy();
  }

  fk::algorithms::klog("ATA CONTROLLER",
                       "Detection complete. Found %d devices.",
                       m_devices.size());
}

void ATAController::detect_on_pci(const PciDevice &device) {
  fk::algorithms::klog("ATA CONTROLLER",
                       "Found PCI IDE controller at %02x:%02x.%d (ProgIF: %02x)",
                       device.address().bus(), device.address().device(),
                       device.address().function(), device.prog_if());

  uint16_t primary_io = 0x1F0;
  uint16_t primary_ctrl = 0x3F6;
  uint16_t secondary_io = 0x170;
  uint16_t secondary_ctrl = 0x376;

  if (device.prog_if() & 0x01) { // Native mode primary
    primary_io = PciManager::the().read_config_dword(device.address(), 0x10) & 0xFFFC;
    primary_ctrl = PciManager::the().read_config_dword(device.address(), 0x14) & 0xFFFC;
    if (primary_ctrl != 0) primary_ctrl += 2;
  }

  if (device.prog_if() & 0x04) { // Native mode secondary
    secondary_io = PciManager::the().read_config_dword(device.address(), 0x18) & 0xFFFC;
    secondary_ctrl = PciManager::the().read_config_dword(device.address(), 0x1C) & 0xFFFC;
    if (secondary_ctrl != 0) secondary_ctrl += 2;
  }

  probe_channel(primary_io, primary_ctrl, 0);
  probe_channel(secondary_io, secondary_ctrl, 1);
}

void ATAController::detect_legacy() {
  probe_channel(0x1F0, 0x3F6, 0);
  probe_channel(0x170, 0x376, 1);
}

void ATAController::probe_channel(uint16_t io, uint16_t ctrl, int channel_index) {
  if (io == 0 || ctrl == 0) return;

  for (int master = 0; master < 2; ++master) {
    bool is_master = (master == 0);

    fk::algorithms::klog("ATA CONTROLLER",
                         "Probing %s on channel %d (IO: %04x, CTRL: %04x)...",
                         is_master ? "Master" : "Slave", channel_index, io, ctrl);

    outb(io + 6, is_master ? 0xA0 : 0xB0);
    outb(io + 2, 0);
    outb(io + 3, 0);
    outb(io + 4, 0);
    outb(io + 5, 0);
    outb(io + 7, 0xEC); // IDENTIFY

    uint8_t status = inb(io + 7);
    if (status == 0) continue;

    while (inb(io + 7) & 0x80);

    if (inb(io + 4) != 0 || inb(io + 5) != 0) continue;

    while (!(inb(io + 7) & 0x08));

    uint16_t data[256];
    for (int j = 0; j < 256; ++j) {
      data[j] = inw(io);
    }

    uint64_t sectors = 0;
    if (data[83] & (1 << 10)) { // LBA48 support
      if (data[100] == 0 && data[101] == 0 && data[102] == 0 && data[103] == 0) {
        fk::algorithms::kwarn("ATA", "Invalid LBA48 sector count - all zeros");
        continue;
      }
      sectors = (uint64_t)data[100] | ((uint64_t)data[101] << 16) |
                ((uint64_t)data[102] << 32) | ((uint64_t)data[103] << 48);
    }
    if (!(data[83] & (1 << 10))) { // LBA28 support
      if (data[60] == 0 && data[61] == 0) {
        fk::algorithms::kwarn("ATA", "Invalid LBA28 sector count - all zeros");
        continue;
      }
      sectors = (uint64_t)data[60] | ((uint64_t)data[61] << 16);
    }

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

    auto strategy = fk::memory::make_owned<PIOStrategy>(io, ctrl, is_master);
    if (!strategy) continue;

    fk::memory::OwnPtr<ATATransferStrategy> transfer_strategy(strategy.leak_ptr());
    auto dev_res = fk::make_ref<ATADevice>(bsd_name, fk::types::move(transfer_strategy), SectorCount(sectors));
    if (dev_res.is_error()) continue;
    
    auto dev = fk::types::move(dev_res.value());
    m_devices.push_back(dev);

    fk::text::String device_name = bsd_name;
    fk::RefPtr<Node> node_ref = dev;
    fkernel::DevFs::the().register_device(node_ref, device_name.c_str());

    PartitionManager::the().scan(dev);

    if (!PartitionManager::the().has_partitions_for_device(dev)) {
      fk::algorithms::klog("ATA", "[%s] No partitions found, trying raw mount...", bsd_name.c_str());
      fkernel::AutoMounter::try_mount(dev);
    }
    if (PartitionManager::the().has_partitions_for_device(dev)) {
      fk::algorithms::klog("ATA", "[%s] Partitions found, skipping raw mount.", bsd_name.c_str());
    }
  }
}