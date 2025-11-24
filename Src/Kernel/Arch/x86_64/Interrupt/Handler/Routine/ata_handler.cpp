#include "Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h"
#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Algorithms/log.h>

// Base I/O addresses for ATA primary and secondary channels
#define ATA_PRIMARY_BASE 0x1F0
#define ATA_SECONDARY_BASE 0x170

// ATA register offsets
#define ATA_REG_STATUS 0x07

void ata_primary_handler(uint8_t vector, InterruptFrame *frame) {
  fk::algorithms::kdebug("INTERRUPT ROUTINE", "Triggering ATA PRIMARY PORT");
  (void)frame;
  inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
  HardwareInterruptManager::the().send_eoi(vector);
}

void ata_secondary_handler(uint8_t vector, InterruptFrame *frame) {
  fk::algorithms::kdebug("INTERRUPT ROUTINE", "Triggering ATA SECONDARY PORT");
  (void)frame;
  fk::algorithms::kdebug("INTERRUPT", "Interrupt triggered: %s",
                         __PRETTY_FUNCTION__);
  inb(ATA_SECONDARY_BASE + ATA_REG_STATUS);
  HardwareInterruptManager::the().send_eoi(vector);
}
