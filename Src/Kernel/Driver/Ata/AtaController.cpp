#ifdef __x86_64
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/io.h>
#endif

#include <Kernel/Block/BlockDevice.h> // Include the new BlockDevice header
#include <Kernel/Block/PartitionDevice.h>
#include <Kernel/Block/PartitionEntry.h>

#include <Kernel/Block/Partition/MbrPartition.h>
#include <Kernel/Block/PartitionManager.h>

#include <Kernel/Driver/Ata/AtaBlockCache.h>
#include <Kernel/Driver/Ata/AtaBlockDevice.h>
#include <Kernel/Driver/Ata/AtaController.h>
#include <Kernel/Driver/Ata/AtaDefs.h>

#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>

#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/own_ptr.h>    // For adopt_own
#include <LibFK/Memory/retain_ptr.h> // For adopt_retain

void ata_irq_primary() {
  fk::algorithms::klog("ATA", "Primary channel IRQ fired!");
  inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
}

void ata_irq_secondary() {
  fk::algorithms::klog("ATA", "Secondary channel IRQ fired!");
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
  fk::algorithms::klog("ATA", "Initializing ATA controller...");
  detect_devices();
  fk::algorithms::klog("ATA", "ATA controller initialization complete.");
}

void AtaController::detect_devices() {
  fk::algorithms::klog("ATA", "Starting ATA device detection...");
  const char *bus_str[] = {"Primary", "Secondary"};
  const char *drive_str[] = {"Master", "Slave"};

  int device_index = 0;

  for (int b = 0; b < 2; ++b) {
    for (int d = 0; d < 2; ++d) {

      AtaDeviceInfo device_info;
      if (!identify_device(static_cast<Bus>(b), static_cast<Drive>(d),
                           device_info)) {
        fk::algorithms::kdebug("ATA", "No device found on %s %s, skipping.",
                               bus_str[b], drive_str[d]);
        continue;
      }

      if (device_info.model[0] == '\0') {
        fk::algorithms::klog("ATA", "%s %s: No device detected, skipping",
                             bus_str[b], drive_str[d]);
        continue;
      }

      fk::algorithms::klog("ATA", "Detected %s %s (ada%d): Model '%s'.",
                           bus_str[b], drive_str[d], device_index,
                           device_info.model);

      fkernel::drivers::ata::AtaDeviceIoInfo io_info(
          static_cast<Bus>(b), static_cast<Drive>(d),
          base_io(static_cast<Bus>(b)), ctrl_io(static_cast<Bus>(b)));

      fk::memory::OwnPtr<fkernel::drivers::ata::AtaIoStrategy> pio_strategy =
          fk::memory::adopt_own(new fkernel::drivers::ata::AtaPioStrategy());

      fk::memory::RetainPtr<AtaBlockDevice> ata_block_dev =
          fk::memory::adopt_retain(new AtaBlockDevice(
              fk::types::move(io_info), fk::types::move(pio_strategy)));

      // Store the retained device in the controller's list to manage its
      // lifetime.
      m_devices.push_back(fk::memory::RetainPtr<BlockDevice>(ata_block_dev.get()));

      // DevFS registration for the raw ATA device is now handled by FilesystemManager
      // Partition detection and filesystem mounting are also handled by FilesystemManager

      device_index++;
    }
  }
  fk::algorithms::klog(
      "ATA", "ATA device detection finished. Total devices found: %d.",
      device_index);
}

bool AtaController::identify_device(Bus bus, Drive drive, AtaDeviceInfo &out) {
  fk::algorithms::klog("ATA",
                       "Attempting to identify device on Bus: %d, Drive: %d.",
                       static_cast<int>(bus), static_cast<int>(drive));
  uint16_t io_base = base_io(bus);
  uint16_t ctrl = ctrl_io(bus);
  uint8_t status;

  (void)ctrl; // Currently unused, silence warning

  // Select drive
  outb(io_base + ATA_REG_HDDEVSEL,
       0xA0 | ((drive == Drive::Slave) ? 0x10 : 0x00));
  io_wait();
  fk::algorithms::kdebug("ATA", "Selected drive 0x%lx.",
                         0xA0 | ((drive == Drive::Slave) ? 0x10 : 0x00));

  // Clear LBA registers
  outb(io_base + ATA_REG_SECCOUNT, 0);
  outb(io_base + ATA_REG_LBA0, 0);
  outb(io_base + ATA_REG_LBA1, 0);
  outb(io_base + ATA_REG_LBA2, 0);
  outb(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
  fk::algorithms::kdebug("ATA", "Sent IDENTIFY command.");

  status = inb(io_base + ATA_REG_STATUS);
  if (status == 0) {
    fk::algorithms::klog(
        "ATA",
        "Device not found on Bus: %d, Drive: %d (status 0 after IDENTIFY).");
    return false;
  }

  // Wait for BSY to clear and DRQ to set (or ERR to set)
  while ((status & ATA_STATUS_BSY) && !(status & ATA_STATUS_ERR)) {
    status = inb(io_base + ATA_REG_STATUS);
    io_wait();
  }
  fk::algorithms::kdebug("ATA", "Device status after BSY clear: 0x%lx.",
                         status);

  if (status & ATA_STATUS_ERR) {
    fk::algorithms::kwarn(
        "ATA",
        "Error during IDENTIFY command on Bus: %d, Drive: %d. Status: 0x%lx.",
        static_cast<int>(bus), static_cast<int>(drive), status);
    return false;
  }

  // Wait for DRQ to set
  while (!(status & ATA_STATUS_DRQ)) {
    status = inb(io_base + ATA_REG_STATUS);
    io_wait();
  }
  fk::algorithms::kdebug("ATA", "Device status after DRQ set: 0x%lx.", status);

  uint16_t id_data[256];
  for (int i = 0; i < 256; ++i)
    id_data[i] = inw(io_base + ATA_REG_DATA);

  // Extract model name
  for (int i = 0; i < 40; i += 2) {
    out.model[i] = (id_data[27 + i / 2] >> 8) & 0xFF;
    out.model[i + 1] = id_data[27 + i / 2] & 0xFF;
  }
  out.model[40] = '\0'; // Null-terminate the string

  out.exists = true;
  out.bus = bus;
  out.drive = drive;

  fk::algorithms::klog(
      "ATA", "Successfully identified device '%s' on Bus: %d, Drive: %d.",
      out.model, static_cast<int>(bus), static_cast<int>(drive));
  return true;
}

int AtaController::read_sectors_pio_impl(Bus bus, Drive drive, uint32_t lba,
                                         uint8_t sector_count, void *buffer) {
  fk::algorithms::klog(
      "ATA PIO", "Reading %u sectors from LBA %u on Bus: %d, Drive: %d.",
      sector_count, lba, static_cast<int>(bus), static_cast<int>(drive));
  uint16_t base = (bus == Bus::Primary) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
  uint8_t head = 0xE0 | ((drive == Drive::Slave) << 4) | ((lba >> 24) & 0x0F);

  outb(base + 6, head);
  io_wait();
  outb(base + 1, sector_count);
  outb(base + 2, lba & 0xFF);
  outb(base + 3, (lba >> 8) & 0xFF);
  outb(base + 4, (lba >> 16) & 0xFF);
  outb(base + 7, ATA_CMD_READ_PIO); // Command READ SECTORS
  fk::algorithms::kdebug("ATA PIO",
                         "Sent READ PIO command for LBA %u, count %u.", lba,
                         sector_count);

  uint8_t status;
  while (!((status = inb(base + 7)) & ATA_STATUS_DRQ) &&
         !(status & ATA_STATUS_ERR)) {
    io_wait();
  }

  if (status & ATA_STATUS_ERR) {
    fk::algorithms::kerror(
        "ATA PIO", "Error status 0x%lx during PIO read for LBA %u, count %u.",
        status, lba, sector_count);
    return -1;
  }

  // Use a temporary byte buffer for reading to avoid alignment issues with the
  // destination buffer.
  size_t total_bytes = sector_count * 512;
  fk::memory::OwnPtr<uint8_t[]> temp_buffer_own_ptr =
      fk::memory::adopt_own<uint8_t[]>(new uint8_t[total_bytes]);
  uint16_t *temp_word_buffer =
      reinterpret_cast<uint16_t *>(temp_buffer_own_ptr.ptr());

  if (!temp_word_buffer) {
    fk::algorithms::kerror("ATA PIO",
                           "Failed to allocate temporary buffer for PIO read.");
    return -1;
  }

  for (size_t i = 0; i < total_bytes / 2; ++i) { // Read words
    temp_word_buffer[i] = inw(base + ATA_REG_DATA);
  }

  // Copy data from temporary buffer to the user-provided buffer
  memcpy(buffer, temp_buffer_own_ptr.ptr(), total_bytes);

  fk::algorithms::klog("ATA PIO", "Successfully read %u sectors from LBA %u.",
                       sector_count, lba);

  return sector_count;
}

int AtaController::write_sectors_pio_impl(Bus bus, Drive drive, uint32_t lba,
                                          uint8_t sector_count,
                                          const void *buffer) {
  fk::algorithms::klog(
      "ATA PIO", "Writing %u sectors to LBA %u on Bus: %d, Drive: %d.",
      sector_count, lba, static_cast<int>(bus), static_cast<int>(drive));
  uint16_t base = base_io(bus);
  uint8_t head = 0xE0 | ((drive == Drive::Slave) << 4) | ((lba >> 24) & 0x0F);

  outb(base + ATA_REG_HDDEVSEL, head);
  io_wait();

  outb(base + ATA_REG_SECCOUNT, sector_count);
  outb(base + ATA_REG_LBA0, lba & 0xFF);
  outb(base + ATA_REG_LBA1, (lba >> 8) & 0xFF);
  outb(base + ATA_REG_LBA2, (lba >> 16) & 0xFF);

  outb(base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
  fk::algorithms::kdebug("ATA PIO",
                         "Sent WRITE PIO command for LBA %u, count %u.", lba,
                         sector_count);

  uint8_t status;
  // Wait for DRQ to set (Data Request Ready)
  while (!((status = inb(base + ATA_REG_STATUS)) & ATA_STATUS_DRQ) &&
         !(status & ATA_STATUS_ERR)) {
    io_wait();
  }

  if (status & ATA_STATUS_ERR) {
    fk::algorithms::kerror(
        "ATA PIO",
        "Error status 0x%lx during PIO write (DRQ wait) for LBA %u, count %u.",
        status, lba, sector_count);
    return -1;
  }

  fk::algorithms::kdebug("ATA PIO",
                         "Starting data transfer for write operation.");
  const uint16_t *data = reinterpret_cast<const uint16_t *>(buffer);
  for (int i = 0; i < 256 * sector_count; ++i) {
    outw(base + ATA_REG_DATA, data[i]);
  }
  fk::algorithms::kdebug("ATA PIO",
                         "Data transfer complete. Waiting for BSY to clear.");

  // Wait for BSY to clear
  while (inb(base + ATA_REG_STATUS) & ATA_STATUS_BSY) {
    io_wait();
  }
  status = inb(base + ATA_REG_STATUS);
  if (status & ATA_STATUS_ERR) {
    fk::algorithms::kerror(
        "ATA PIO", "Error status 0x%lx after PIO write for LBA %u, count %u.",
        status, lba, sector_count);
    return -1;
  }

  fk::algorithms::klog("ATA PIO", "Successfully wrote %u sectors to LBA %u.",
                       sector_count, lba);
  return sector_count; // Return sector count, not bytes written, to match
                       // read_sectors_pio_impl and BlockDevice interface.
}
