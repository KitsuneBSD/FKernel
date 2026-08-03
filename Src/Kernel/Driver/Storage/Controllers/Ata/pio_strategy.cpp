#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Driver/Storage/Controllers/Ata/pio_strategy.h>
#include <LibFK/Algorithms/Logging/log.h>

static constexpr uint8_t ATA_REG_DATA = 0;
static constexpr uint8_t ATA_REG_SECCOUNT = 2;
static constexpr uint8_t ATA_REG_LBA0 = 3;
static constexpr uint8_t ATA_REG_LBA1 = 4;
static constexpr uint8_t ATA_REG_LBA2 = 5;
static constexpr uint8_t ATA_REG_HDDEVSEL = 6;
static constexpr uint8_t ATA_REG_COMMAND = 7;
static constexpr uint8_t ATA_REG_STATUS = 7;

static constexpr uint8_t ATA_CMD_READ_PIO = 0x20;
static constexpr uint8_t ATA_CMD_WRITE_PIO = 0x30;
static constexpr uint8_t ATA_CMD_CACHE_FLUSH = 0xE7;

static constexpr uint8_t ATA_SR_BSY = 0x80;
static constexpr uint8_t ATA_SR_DRDY = 0x40;
static constexpr uint8_t ATA_SR_DRQ = 0x08;
static constexpr uint8_t ATA_SR_ERR = 0x01;

void PIOStrategy::wait_busy() { wait_status(ATA_SR_BSY, 0); }

void PIOStrategy::wait_ready() { wait_status(ATA_SR_DRDY, ATA_SR_DRDY); }

bool PIOStrategy::wait_status(uint8_t mask, uint8_t value,
                              uint32_t timeout_loops) {
  for (uint32_t i = 0; i < timeout_loops; ++i) {
    uint8_t status = inb(m_io_base + ATA_REG_STATUS);
    if (status == 0xFF)
      return false; // Floating bus
    if ((status & mask) == value)
      return true;
    if (status & ATA_SR_ERR)
      return false;
    arch_cpu_relax();
  }
  return false;
}

void PIOStrategy::select_drive(uint64_t lba, uint8_t count) {
  outb(m_io_base + ATA_REG_HDDEVSEL,
       (m_is_master ? 0xE0 : 0xF0) | ((lba >> 24) & 0x0F));
  outb(m_io_base + ATA_REG_SECCOUNT, count);
  outb(m_io_base + ATA_REG_LBA0, (uint8_t)lba);
  outb(m_io_base + ATA_REG_LBA1, (uint8_t)(lba >> 8));
  outb(m_io_base + ATA_REG_LBA2, (uint8_t)(lba >> 16));
}

fk::core::Result<size_t, fk::core::Error>
PIOStrategy::read_sectors(uint64_t start_sector, size_t count,
                          uint8_t *buffer) {
  if (!buffer || count == 0) {
    return fk::core::Error::InvalidParameter;
  }
  
  fk::algorithms::klog("PIO", "Executing read. LBA: %lu, count: %zu",
                       start_sector, count);

  uint16_t *ptr = (uint16_t *)buffer;
  for (size_t i = 0; i < count; ++i) {
    if (!wait_status(ATA_SR_BSY, 0))
      return fk::core::Error::IOError;

    select_drive(start_sector + i, 1);
    outb(m_io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    if (!wait_status(ATA_SR_DRQ, ATA_SR_DRQ)) {
      fk::algorithms::kerror(
          "PIO", "Error or timeout waiting for DRQ during read at LBA %lu",
          start_sector + i);
      return fk::core::Error::IOError;
    }

    insw(m_io_base + ATA_REG_DATA, ptr, 256);
    ptr += 256;
  }

  return count;
}

fk::core::Result<size_t, fk::core::Error>
PIOStrategy::write_sectors(uint64_t start_sector, size_t count,
                           const uint8_t *buffer) {
  if (!buffer || count == 0) {
    return fk::core::Error::InvalidParameter;
  }
  
  fk::algorithms::klog("PIO", "Executing write. LBA: %lu, count: %zu",
                       start_sector, count);

  const uint16_t *ptr = (const uint16_t *)buffer;
  for (size_t i = 0; i < count; ++i) {
    if (!wait_status(ATA_SR_BSY, 0))
      return fk::core::Error::IOError;

    select_drive(start_sector + i, 1);
    outb(m_io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    if (!wait_status(ATA_SR_DRQ, ATA_SR_DRQ)) {
      fk::algorithms::kerror(
          "PIO", "Error or timeout waiting for DRQ during write at LBA %lu",
          start_sector + i);
      return fk::core::Error::IOError;
    }

    for (int j = 0; j < 256; ++j) {
      outw(m_io_base + ATA_REG_DATA, ptr[j]);
    }
    ptr += 256;
  }

  outb(m_io_base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
  wait_status(ATA_SR_BSY, 0);

  return count;
}
