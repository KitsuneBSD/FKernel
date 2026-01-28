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
  // Manual call to instantiate drivers - this will trigger detect_on_pci 
  // for any detected PCI IDE controllers.
  PciManager::the().instantiate_drivers();

  // If no devices were found via PCI, fallback to legacy only if ACPI says it's okay
  if (m_devices.is_empty()) {
    fk::algorithms::klog("ATA CONTROLLER", "No IDE devices found via PCI, trying legacy...");
    detect_legacy();
  }

  fk::algorithms::klog("ATA CONTROLLER",
                       "Detection complete. Total devices: %d",
                       m_devices.size());
}

void ATAController::create(const PciDevice& device) {
  the().detect_on_pci(device);
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

  // Read BARs if in native mode
  if (device.prog_if() & 0x01) { // Native mode primary
    uint32_t bar0 = PciManager::the().read_config_dword(device.address(), 0x10);
    uint32_t bar1 = PciManager::the().read_config_dword(device.address(), 0x14);
    if (bar0 != 0) primary_io = bar0 & 0xFFFC;
    if (bar1 != 0) primary_ctrl = (bar1 & 0xFFFC) + 2;
  }

  if (device.prog_if() & 0x04) { // Native mode secondary
    uint32_t bar2 = PciManager::the().read_config_dword(device.address(), 0x18);
    uint32_t bar3 = PciManager::the().read_config_dword(device.address(), 0x1C);
    if (bar2 != 0) secondary_io = bar2 & 0xFFFC;
    if (bar3 != 0) secondary_ctrl = (bar3 & 0xFFFC) + 2;
  }

  probe_channel(primary_io, primary_ctrl, 0);
  probe_channel(secondary_io, secondary_ctrl, 1);
}

void ATAController::detect_legacy() {
  fk::algorithms::klog("ATA CONTROLLER", "Probing legacy IO ports (0x1F0, 0x170)...");
  // Optional: check ACPI FADT 8042 flag or similar if needed
  // For now, just probe standard ports as a last resort
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

    // Register with DriverManager (which also handles DevFs)
    fkernel::DriverManager::the().register_device(dev);

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