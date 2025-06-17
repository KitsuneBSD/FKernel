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
  uint8_t index = 0x10 + irq * 2;
  uint32_t low = vector;
  uint32_t high = apic_id << 24;

  ioapic_write(index + 1, high);
  ioapic_write(index, low);
}
