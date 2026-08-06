#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/io.h>

static constexpr uint16_t ATA_REG_STATUS = 0x07;

void ata_primary_handler(uint8_t vector, InterruptFrame *frame) {
  (void)frame;
  // Acknowledge interrupt on legacy primary channel
  // Note: In a fully dynamic system, these should be retrieved from the controller instance
  uint16_t primary_base = 0x1F0;
  inb(primary_base + ATA_REG_STATUS);

  fk::algorithms::klog("ATA", "Primary channel interrupt handled (vector 0x%x)", vector);
  HardwareInterruptManager::the().send_eoi(vector);
}

void ata_secondary_handler(uint8_t vector, InterruptFrame *frame) {
  (void)frame;
  // Acknowledge interrupt on legacy secondary channel
  uint16_t secondary_base = 0x170;
  inb(secondary_base + ATA_REG_STATUS);

  fk::algorithms::klog("ATA", "Secondary channel interrupt handled (vector 0x%x)", vector);
  HardwareInterruptManager::the().send_eoi(vector);
}
