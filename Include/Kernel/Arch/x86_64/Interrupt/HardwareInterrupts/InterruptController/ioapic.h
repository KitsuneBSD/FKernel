#pragma once

#include "LibFK/Container/string.h"
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <LibFK/Types/types.h>

// I/O APIC Registers
constexpr uint32_t IOAPIC_REG_ID = 0x00;
constexpr uint32_t IOAPIC_REG_VER = 0x01;
constexpr uint32_t IOAPIC_REG_ARB = 0x02;
constexpr uint32_t IOAPIC_REG_TABLE_BASE = 0x10;
constexpr uintptr_t IOAPIC_ADDRESS = 0xFEC00000;

/**
 * @brief I/O APIC controller implementing HardwareInterrupt interface
 */
class IOAPIC : public HardwareInterrupt {
private:
  uintptr_t ioapic_base = 0;
  // uint32_t global_interrupt_base = 0;

  String m_name = "IOAPIC";

  uint32_t read(uint32_t reg);
  void write(uint32_t reg, uint32_t value);

public:
  IOAPIC() = default;

  String get_name() override { return m_name; }
  void initialize() override;
  void mask_interrupt(uint8_t irq) override;
  void unmask_interrupt(uint8_t irq) override;
  void send_eoi(uint8_t irq) override;

  /**
   * @brief Remap IRQ to vector and LAPIC
   */
  void remap_irq(uint8_t irq, uint8_t vector, uint8_t lapic_id, uint32_t flags);
};
