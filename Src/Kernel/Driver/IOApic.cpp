#include <Driver/IOApic.h>

volatile uint32_t *ioapic_reg = (volatile uint32_t *)IOAPIC_BASE;
volatile uint32_t *ioapic_data = (volatile uint32_t *)(IOAPIC_BASE + 0x10);

void ioapic_write(uint8_t reg, uint32_t val) {
  *ioapic_reg = reg;
  *ioapic_data = val;
}

uint32_t ioapic_read(uint8_t reg) {
  *ioapic_reg = reg;
  return *ioapic_data;
}

void ioapic_redirect_irq(uint8_t irq, uint8_t vector, uint8_t apic_id) {
  if (irq >= 24)
    return;

  uint32_t low = 0;
  low |= vector;
  low |= 0 << 8;
  low |= 0 << 11;
  low |= 0 << 13;
  low |= 0 << 15;
  low |= 0 << 16;

  uint32_t high = ((uint32_t)apic_id) << 24;

  uint8_t redir_index = 0x10 + irq * 2;
  ioapic_write(redir_index + 1, high);
  ioapic_write(redir_index, low);
}
