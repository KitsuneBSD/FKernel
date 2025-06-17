#pragma once

#include <LibC/stdint.h>

#define IOAPIC_BASE 0xFEC00000

void ioapic_redirect_irq(uint8_t irq, uint8_t vector, uint8_t apic_id = 0);

void ioapic_write(uint8_t reg, uint32_t val);

uint32_t ioapic_read(uint8_t reg);
