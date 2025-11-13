#ifdef __x86_64
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/io.h>
#endif

#include <Kernel/Block/BlockDevice.h> // Include the new BlockDevice header
#include <Kernel/Block/PartitionEntry.h>
#include <Kernel/Block/PartitionDevice.h>

#include <Kernel/Block/Partition/BsdPartition.h>
#include <Kernel/Block/Partition/MbrPartition.h>

#include <Kernel/Driver/Ata/AtaBlockCache.h>
#include <Kernel/Driver/Ata/AtaBlockDevice.h>
#include <Kernel/Driver/Ata/AtaController.h>
#include <Kernel/Driver/Ata/AtaDefs.h> // Missing include

#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>

#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/retain_ptr.h> // For adopt_retain

// TODO: Separe responsabilities between the controller driver and the block
// bootsector

void ata_irq_primary() {
  klog("ATA", "Primary channel IRQ fired!");
  inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
}

void ata_irq_secondary() {
  klog("ATA", "Secondary channel IRQ fired!");
  inb(ATA_SECONDARY_BASE + ATA_REG_STATUS);
}

AtaController &AtaController::the() {
  static AtaController controller;
  return controller;
}

uint16_t AtaController::base_io(Bus bus) {
  return (bus == Bus::Primary) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
}

uint16_t AtaController::ctrl_io(Bus bus) {
  return (bus == Bus::Primary) ? ATA_PRIMARY_CTRL : ATA_SECONDARY_CTRL;
}

void AtaController::initialize() {
  klog("ATA", "Initializing ATA controller...");
  detect_devices();
}

void AtaController::detect_devices() {
  const char *bus_str[] = {"Primary", "Secondary"};
  const char *drive_str[] = {"Master", "Slave"};

  int device_index = 0;
  for (int b = 0; b < 2; ++b) {
    for (int d = 0; d < 2; ++d) {
      AtaDeviceInfo device_info; // Create a temporary AtaDeviceInfo
      if (identify_device(static_cast<Bus>(b), static_cast<Drive>(d), device_info)) {
        klog("ATA", "Detected %s %s: Model '%s'", bus_str[b], drive_str[d],
             device_info.model);

        // Create an AtaBlockDevice instance using the detected info
        RetainPtr<AtaBlockDevice> ata_block_dev = adopt_retain(new AtaBlockDevice(device_info));

        char name[16];
        snprintf(name, sizeof(name), "ada%d", device_index);

        DevFS::the().register_device("ada", VNodeType::BlockDevice,
                                     &g_block_device_ops, ata_block_dev.leakRef(), true); // Use global ops and instance

        device_index++;
      }
    }
  }
}

bool AtaController::identify_device(Bus bus, Drive drive, AtaDeviceInfo &out) {
  uint16_t io_base = base_io(bus);
  uint16_t ctrl = ctrl_io(bus);
  uint8_t status;

  (void)ctrl;

  outb(io_base + ATA_REG_HDDEVSEL,
       0xA0 | ((drive == Drive::Slave) ? 0x10 : 0x00));
  io_wait();
  outb(io_base + ATA_REG_SECCOUNT, 0);
  outb(io_base + ATA_REG_LBA0, 0);
  outb(io_base + ATA_REG_LBA1, 0);
  outb(io_base + ATA_REG_LBA2, 0);
  outb(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

  status = inb(io_base + ATA_REG_STATUS);
  if (status == 0)
    return false;

  while ((status & ATA_STATUS_BSY) && !(status & ATA_STATUS_ERR))
    status = inb(io_base + ATA_REG_STATUS);

  if (status & ATA_STATUS_ERR)
    return false;

  // Espera DRQ
  while (!(status & ATA_STATUS_DRQ))
    status = inb(io_base + ATA_REG_STATUS);

  uint16_t id_data[256];
  for (int i = 0; i < 256; ++i)
    id_data[i] = inw(io_base + ATA_REG_DATA);

  for (int i = 0; i < 40; i += 2) {
    out.model[i] = (id_data[27 + i / 2] >> 8) & 0xFF;
    out.model[i + 1] = id_data[27 + i / 2] & 0xFF;
  }
  out.model[40] = '\0';
  out.exists = true;
  out.bus = bus;
  out.drive = drive;
  return true;
}

int AtaController::read_sectors_pio(Bus bus, Drive drive, uint32_t lba,
                                    uint8_t sector_count, void *buffer) {
  uint16_t base = (bus == Bus::Primary) ? 0x1F0 : 0x170;
  uint8_t head = 0xE0 | ((drive == Drive::Slave) << 4) | ((lba >> 24) & 0x0F);
  outb(base + 6, head);

  outb(base + 1, sector_count);
  outb(base + 2, lba & 0xFF);
  outb(base + 3, (lba >> 8) & 0xFF);
  outb(base + 4, (lba >> 16) & 0xFF);
  outb(base + 7, 0x20); // Comando READ SECTORS

  while (!(inb(base + 7) & 0x08)) {
  }

  for (int i = 0; i < 256 * sector_count; ++i) {
    ((uint16_t *)buffer)[i] = inw(base);
  }

  return sector_count;
}

int AtaController::read_sectors_pio(const AtaDeviceInfo &device, uint32_t lba,
                                    uint8_t count, void *buffer) {
  return read_sectors_pio(device.bus, device.drive, lba, count, buffer);
}

int AtaController::write_sectors_pio(const AtaDeviceInfo &device, uint32_t lba,
                                     uint8_t count, const void *buffer) {
  return write_sectors_pio(device.bus, device.drive, lba, count, buffer);
}

int AtaController::write_sectors_pio(Bus bus, Drive drive, uint32_t lba,
                                     uint8_t sector_count, const void *buffer) {
  uint16_t base = base_io(bus);
  uint8_t head = 0xE0 | ((drive == Drive::Slave) << 4) | ((lba >> 24) & 0x0F);

  outb(base + ATA_REG_HDDEVSEL, head);
  io_wait();

  outb(base + ATA_REG_SECCOUNT, sector_count);
  outb(base + ATA_REG_LBA0, lba & 0xFF);
  outb(base + ATA_REG_LBA1, (lba >> 8) & 0xFF);
  outb(base + ATA_REG_LBA2, (lba >> 16) & 0xFF);

  outb(base + ATA_REG_COMMAND, 0x30);

  // Espera DRQ
  while (!(inb(base + ATA_REG_STATUS) & ATA_STATUS_DRQ))
    ;

  const uint16_t *data = reinterpret_cast<const uint16_t *>(buffer);
  for (int i = 0; i < 256 * sector_count; ++i) {
    outw(base + ATA_REG_DATA, data[i]);
  }

  while (inb(base + ATA_REG_STATUS) & ATA_STATUS_BSY)
    ;

  return sector_count * 512;
}
